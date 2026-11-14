#include "kaleidoscope/runtime/DeviceRuntime.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <dlfcn.h>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iomanip>
#include <iostream>
#include <memory>
#include <numeric>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>
using Clock=std::chrono::steady_clock;
double ms(Clock::time_point t) {return std::chrono::duration<double,std::milli>(Clock::now()-t).count();}
struct Library {
    void *handle;
    explicit Library(const char *path):handle(dlopen(path,RTLD_NOW|RTLD_LOCAL)) {
        if(!handle) throw std::runtime_error(dlerror());
    }
    ~Library(){dlclose(handle);}
    template<class T> T symbol(const char *name) {
        dlerror(); auto value=dlsym(handle,name);
        if(auto *e=dlerror()) throw std::runtime_error(e);
        return reinterpret_cast<T>(value);
    }
};
struct Session {
    KrtSession *s=nullptr;
    Session(const char *backend) {
        int status=krt_create(backend,&s);
        if(status!=KRT_OK) throw std::runtime_error(std::string("Runtime unavailable: ")+backend+" status="+std::to_string(status));
    }
    ~Session(){krt_destroy(s);}
    void check(int status){if(status!=KRT_OK) throw std::runtime_error(krt_error(s));}
    KrtStats stats(){KrtStats result{};check(krt_stats(s,&result));return result;}
};
struct CSV {
    std::ofstream out;
    explicit CSV(const char *file) {
        bool header=!std::filesystem::exists(file) || std::filesystem::file_size(file)==0;
        out.open(file,std::ios::app);
        if(!out) throw std::runtime_error("Cannot open CSV");
        out<<std::setprecision(12);
        if(header) out<<"group,variant,n,iterations,mode,sample,batch,wall_ms,h2d_ms,kernel_ms,d2h_ms,allocations,checksum\n";
    }
    void row(const std::string &group,int variant,size_t n,int iterations,const std::string &mode,
             int sample,int batch,double wall,KrtStats delta={},double checksum=0) {
        out<<group<<','<<variant<<','<<n<<','<<iterations<<','<<mode<<','<<sample<<','<<batch<<','
           <<wall/batch<<','<<delta.h2d_ms/batch<<','<<delta.kernel_ms/batch<<','<<delta.d2h_ms/batch
           <<','<<double(delta.allocations)/batch<<','<<checksum<<'\n';
        if(!out) throw std::runtime_error("CSV write failed");
    }
};
KrtStats diff(KrtStats a,KrtStats b){
    return {a.h2d_ms-b.h2d_ms,a.kernel_ms-b.kernel_ms,a.d2h_ms-b.d2h_ms,
            a.allocations-b.allocations,a.live_allocations,a.launches-b.launches};
}
struct Arrays {
    size_t n;
    std::vector<double> x,y,out,expected;
    KaleArray dx,dy,dout;
    Arrays(size_t count,int repetitions):n(count),x(n+1),y(n+1),out(n+1,-9876),expected(n) {
        for(size_t i=0;i<n;++i) {
            x[i]=(int(i%97)-48)*.125;y[i]=(int(i%31)-15)*.03125;
            double v=x[i]; for(int j=0;j<repetitions;++j) v=v*.999+y[i];
            expected[i]=v;
        }
        dx={x.data(),x.data(),0,{int64_t(n)},{1}};
        dy={y.data(),y.data(),0,{int64_t(n)},{1}};
        dout={out.data(),out.data(),0,{int64_t(n)},{1}};
    }
    double verify() {
        double checksum=0;
        for(size_t i=0;i<n;++i){
            if(!std::isfinite(out[i]) || std::abs(out[i]-expected[i])>1e-11*(1+std::abs(expected[i])))
                throw std::runtime_error("Numerical mismatch at "+std::to_string(i));
            checksum+=out[i];
        }
        if(out[n]!=-9876) throw std::runtime_error("Tail guard overwritten");
        return checksum;
    }
    void enter(Session &s){
        for(void *p:{static_cast<void*>(x.data()),static_cast<void*>(y.data()),static_cast<void*>(out.data())})
            s.check(krt_enter(s.s,p,n*sizeof(double)));
    }
    void copyin(Session &s){s.check(krt_copyin(s.s,x.data()));s.check(krt_copyin(s.s,y.data()));}
    void launch(Session &s){s.check(krt_launch(s.s,x.data(),y.data(),out.data(),.999,n));}
    void copyout(Session &s){s.check(krt_copyout(s.s,out.data()));}
    void exit(Session &s){
        for(void *p:{static_cast<void*>(x.data()),static_cast<void*>(y.data()),static_cast<void*>(out.data())})
            s.check(krt_exit(s.s,p));
    }
    void scope(Session &s){enter(s);copyin(s);launch(s);s.check(krt_wait(s.s));copyout(s);exit(s);}
};
using Vector=double(*)(KaleArray*,KaleArray*,KaleArray*,double);
using Acc=int(*)(KrtSession*,const KaleArray*,const KaleArray*,KaleArray*,double);
using Baseline=void(*)(KaleArray*,KaleArray*,KaleArray*,double);
std::vector<size_t> sizes(const char *text){
    std::stringstream stream(text);std::string part;std::vector<size_t> result;
    while(std::getline(stream,part,',')) result.push_back(std::stoull(part));
    return result;
}
void vectorBench(int argc,char **argv) {
    // vector sharedlib ptx cppshared variant csv samples warm sizes group
    if(argc!=11) throw std::runtime_error("Invalid vector benchmark arguments");
    Library generated(argv[2]),cpp(argv[4]);int variant=std::stoi(argv[5]);
    CSV csv(argv[6]);int samples=std::stoi(argv[7]),warm=std::stoi(argv[8]);
    std::string group=argv[10];
    auto vector=generated.symbol<Vector>("_mlir_ciface_vector");
    auto acc=generated.symbol<Acc>("kacc_vector");
    auto element=generated.symbol<KrtElement>("element");
    auto checked=cpp.symbol<Baseline>("checked");
    auto unchecked=cpp.symbol<Baseline>("unchecked");
    auto begin=Clock::now();Session gpu("cuda");
    csv.row("startup",variant,0,1,"cuda_context",0,1,ms(begin));
    begin=Clock::now();gpu.check(krt_load_ptx(gpu.s,argv[3]));
    csv.row("startup",variant,0,1,"cuda_module",0,1,ms(begin));
    Session resident("cuda");resident.check(krt_load_ptx(resident.s,argv[3]));
    Session cpu("cpu");cpu.check(krt_set_cpu_kernel(cpu.s,element));
    Arrays cold(257,variant);
    auto before=gpu.stats();begin=Clock::now();cold.scope(gpu);
    double wall=ms(begin);csv.row("first_use",variant,257,1,"gpu_scope_manual",0,1,wall,
        diff(gpu.stats(),before),cold.verify());
    std::mt19937 random(20261114);
    for(auto n:sizes(argv[9])){
        Arrays a(n,variant);
        a.enter(resident);a.copyin(resident);
        std::vector<std::string> modes={"cpu_mlir","cpp_checked","cpp_unchecked","cpu_sim_scope",
                                       "gpu_resident","gpu_scope_manual","gpu_scope_directive"};
        for(int sample=-warm;sample<samples;++sample){
            std::shuffle(modes.begin(),modes.end(),random);
            for(auto &mode:modes){
                bool direct=mode=="cpu_mlir" || mode=="cpp_checked" || mode=="cpp_unchecked";
                int batch=direct?(n<=257?1000:n<=65536?10:1):1;
                Session *session=mode=="gpu_resident"?&resident:
                    (mode=="gpu_scope_manual" || mode=="gpu_scope_directive")?&gpu:
                    mode=="cpu_sim_scope"?&cpu:nullptr;
                KrtStats before=session?session->stats():KrtStats{};
                auto t=Clock::now();
                if(mode=="cpu_mlir")for(int i=0;i<batch;++i)vector(&a.dx,&a.dy,&a.dout,.999);
                else if(mode=="cpp_checked")for(int i=0;i<batch;++i)checked(&a.dx,&a.dy,&a.dout,.999);
                else if(mode=="cpp_unchecked")for(int i=0;i<batch;++i)unchecked(&a.dx,&a.dy,&a.dout,.999);
                else if(mode=="gpu_resident")a.launch(resident);
                else if(mode=="gpu_scope_directive")gpu.check(acc(gpu.s,&a.dx,&a.dy,&a.dout,.999));
                else a.scope(*session);
                double elapsed=ms(t);
                auto delta=session?diff(session->stats(),before):KrtStats{};
                // The validation download is outside the resident timing and its stats.
                if(mode=="gpu_resident")a.copyout(resident);
                auto checksum=a.verify();
                if(sample>=0)csv.row(group,variant,n,1,mode,sample,batch,elapsed,delta,checksum);
                if(session && mode!="gpu_resident" && session->stats().live_allocations)
                    throw std::runtime_error("Scope leaked allocation");
            }
        }
        a.exit(resident);
        std::cerr<<group<<" variant="<<variant<<" n="<<n<<" passed\n";
    }
}
void residencyBench(int argc,char **argv){
    // residency sharedlib ptx csv samples warm sizes
    if(argc!=8)throw std::runtime_error("Invalid residency arguments");
    Library generated(argv[2]);CSV csv(argv[4]);Session gpu("cuda");
    gpu.check(krt_load_ptx(gpu.s,argv[3]));
    int samples=std::stoi(argv[5]),warm=std::stoi(argv[6]);
    for(auto n:sizes(argv[7]))for(int k:{1,10,100}){
        Arrays a(n,1);
        for(int sample=-warm;sample<samples;++sample){
            std::vector<std::string> modes=sample%2==0?std::vector<std::string>{"fresh","resident"}:
                std::vector<std::string>{"resident","fresh"};
            for(auto &mode:modes){
                auto before=gpu.stats();auto t=Clock::now();
                if(mode=="fresh"){for(int j=0;j<k;++j)a.scope(gpu);}
                else{
                    a.enter(gpu);a.copyin(gpu);
                    for(int j=0;j<k;++j)a.launch(gpu);
                    gpu.check(krt_wait(gpu.s));a.copyout(gpu);a.exit(gpu);
                }
                auto wall=ms(t);auto delta=diff(gpu.stats(),before);
                auto checksum=a.verify();
                if(sample>=0)csv.row("residency",1,n,k,mode,sample,1,wall,delta,checksum);
            }
        }
        std::cerr<<"residency n="<<n<<" K="<<k<<" passed\n";
    }
}
volatile double sink=0;
void scalarBench(int argc,char **argv){
    if(argc!=7)throw std::runtime_error("Invalid scalar arguments");
    Library oldlib(argv[2]),newlib(argv[3]);CSV csv(argv[4]);
    int samples=std::stoi(argv[5]),warm=std::stoi(argv[6]);
    using F=double(*)(double,double);using I=int32_t(*)(int32_t,int32_t);
    using B=bool(*)(double);using L=int32_t(*)(int32_t);
    F fs[]={oldlib.symbol<F>("scalar"),newlib.symbol<F>("scalar")};
    I is[]={oldlib.symbol<I>("integer"),newlib.symbol<I>("integer")};
    B bs[]={oldlib.symbol<B>("predicate"),newlib.symbol<B>("predicate")};
    L ls[]={oldlib.symbol<L>("loops"),newlib.symbol<L>("loops")};
    for(int i=0;i<100;++i)
        if(fs[0](i*.125,2)!=fs[1](i*.125,2) || is[0](i,17)!=is[1](i,17) ||
           bs[0](i)!=bs[1](i) || ls[0](i)!=ls[1](i))throw std::runtime_error("Scalar parity failed");
    const char *names[]={"scalar","integer","predicate","loops"};
    for(int kind=0;kind<4;++kind){
        int calls=kind==3?200000:2000000;
        for(int sample=-warm;sample<samples;++sample)for(int order=0;order<2;++order){
            int backend=(order+(sample&1))%2;double sum=0;auto t=Clock::now();
            if(kind==0)for(int i=0;i<calls;++i)sum+=fs[backend]((i%97)*.125,1.25);
            else if(kind==1)for(int i=0;i<calls;++i)sum+=is[backend](i%97,17);
            else if(kind==2)for(int i=0;i<calls;++i)sum+=bs[backend]((i%97)-48.);
            else for(int i=0;i<calls;++i)sum+=ls[backend](i%97);
            auto wall=ms(t);sink=sum;
            if(sample>=0)csv.row("scalar",1,calls,1,std::string(names[kind])+"_"+(backend?"mlir":"llvm"),
                sample,calls,wall,{},sum);
        }
    }
}
int main(int argc,char **argv){
    try{
        if(argc<2)throw std::runtime_error("Expected vector, residency or scalar");
        if(std::string(argv[1])=="vector")vectorBench(argc,argv);
        else if(std::string(argv[1])=="residency")residencyBench(argc,argv);
        else if(std::string(argv[1])=="scalar")scalarBench(argc,argv);
        else throw std::runtime_error("Unknown benchmark mode");
        return 0;
    }catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
}
