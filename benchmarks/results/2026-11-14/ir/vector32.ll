; ModuleID = 'Kaleidoscope'
source_filename = "Kaleidoscope"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128"
target triple = "x86_64-pc-linux-gnu"

@assert_msg_1 = private constant [39 x i8] c"Kaleidoscope array index out of bounds\00"
@assert_msg_0 = private constant [39 x i8] c"Kaleidoscope array index out of bounds\00"
@assert_msg = private constant [39 x i8] c"Kaleidoscope array index out of bounds\00"

; Function Attrs: cold nofree noreturn nounwind
declare void @abort() local_unnamed_addr #0

declare void @puts(ptr) local_unnamed_addr

; Function Attrs: mustprogress nofree norecurse nosync nounwind willreturn memory(none)
define double @element(double %0, double %1, double %2) #1 {
  %4 = fmul double %0, %2
  %5 = fadd double %1, %4
  %6 = fmul double %2, %5
  %7 = fadd double %1, %6
  %8 = fmul double %2, %7
  %9 = fadd double %1, %8
  %10 = fmul double %2, %9
  %11 = fadd double %1, %10
  %12 = fmul double %2, %11
  %13 = fadd double %1, %12
  %14 = fmul double %2, %13
  %15 = fadd double %1, %14
  %16 = fmul double %2, %15
  %17 = fadd double %1, %16
  %18 = fmul double %2, %17
  %19 = fadd double %1, %18
  %20 = fmul double %2, %19
  %21 = fadd double %1, %20
  %22 = fmul double %2, %21
  %23 = fadd double %1, %22
  %24 = fmul double %2, %23
  %25 = fadd double %1, %24
  %26 = fmul double %2, %25
  %27 = fadd double %1, %26
  %28 = fmul double %2, %27
  %29 = fadd double %1, %28
  %30 = fmul double %2, %29
  %31 = fadd double %1, %30
  %32 = fmul double %2, %31
  %33 = fadd double %1, %32
  %34 = fmul double %2, %33
  %35 = fadd double %1, %34
  %36 = fmul double %2, %35
  %37 = fadd double %1, %36
  %38 = fmul double %2, %37
  %39 = fadd double %1, %38
  %40 = fmul double %2, %39
  %41 = fadd double %1, %40
  %42 = fmul double %2, %41
  %43 = fadd double %1, %42
  %44 = fmul double %2, %43
  %45 = fadd double %1, %44
  %46 = fmul double %2, %45
  %47 = fadd double %1, %46
  %48 = fmul double %2, %47
  %49 = fadd double %1, %48
  %50 = fmul double %2, %49
  %51 = fadd double %1, %50
  %52 = fmul double %2, %51
  %53 = fadd double %1, %52
  %54 = fmul double %2, %53
  %55 = fadd double %1, %54
  %56 = fmul double %2, %55
  %57 = fadd double %1, %56
  %58 = fmul double %2, %57
  %59 = fadd double %1, %58
  %60 = fmul double %2, %59
  %61 = fadd double %1, %60
  %62 = fmul double %2, %61
  %63 = fadd double %1, %62
  %64 = fmul double %2, %63
  %65 = fadd double %1, %64
  %66 = fmul double %2, %65
  %67 = fadd double %1, %66
  ret double %67
}

define noundef double @vector(ptr nofree readnone captures(none) %0, ptr nofree readonly captures(none) %1, i64 %2, i64 %3, i64 %4, ptr nofree readnone captures(none) %5, ptr nofree readonly captures(none) %6, i64 %7, i64 %8, i64 %9, ptr nofree readnone captures(none) %10, ptr nofree writeonly captures(none) %11, i64 %12, i64 %13, i64 %14, double %15) local_unnamed_addr {
  %.fr = freeze i64 %8
  %17 = ptrtoaddr ptr %1 to i64
  %18 = ptrtoaddr ptr %6 to i64
  %19 = ptrtoaddr ptr %11 to i64
  %20 = trunc i64 %3 to i32
  %21 = icmp sgt i32 %20, 0
  br i1 %21, label %.preheader.preheader, label %.loopexit

.preheader.preheader:                             ; preds = %16
  %smax12 = tail call i64 @llvm.smax.i64(i64 %13, i64 0)
  %22 = add i64 %3, 4294967295
  %23 = and i64 %22, 4294967295
  %umin13 = tail call i64 @llvm.umin.i64(i64 %smax12, i64 %23)
  %24 = freeze i64 %umin13
  %smax14 = tail call i64 @llvm.smax.i64(i64 %.fr, i64 0)
  %umin15 = tail call i64 @llvm.umin.i64(i64 %24, i64 %smax14)
  %smax16 = tail call i64 @llvm.smax.i64(i64 %3, i64 0)
  %umin17 = tail call i64 @llvm.umin.i64(i64 %smax16, i64 %umin15)
  %umin18 = tail call i64 @llvm.umin.i64(i64 %umin17, i64 2147483648)
  %25 = trunc nuw i64 %umin18 to i32
  %min.iters.check = icmp samesign ult i64 %umin17, 2
  br i1 %min.iters.check, label %.preheader.preheader20, label %vector.scevcheck

vector.scevcheck:                                 ; preds = %.preheader.preheader
  %smax = tail call i64 @llvm.smax.i64(i64 %13, i64 0)
  %26 = add i64 %3, 4294967295
  %27 = and i64 %26, 4294967295
  %umin = tail call i64 @llvm.umin.i64(i64 %smax, i64 %27)
  %28 = freeze i64 %umin
  %smax6 = tail call i64 @llvm.smax.i64(i64 %.fr, i64 0)
  %umin7 = tail call i64 @llvm.umin.i64(i64 %28, i64 %smax6)
  %smax8 = tail call i64 @llvm.smax.i64(i64 %3, i64 0)
  %umin9 = tail call i64 @llvm.umin.i64(i64 %smax8, i64 %umin7)
  %29 = icmp samesign ugt i64 %umin9, 2147483647
  br i1 %29, label %.preheader.preheader20, label %vector.memcheck

vector.memcheck:                                  ; preds = %vector.scevcheck
  %30 = sub i64 %18, %19
  %diff.check = icmp ugt i64 %30, -16
  %31 = sub i64 %17, %19
  %diff.check11 = icmp ugt i64 %31, -16
  %conflict.rdx = or i1 %diff.check, %diff.check11
  br i1 %conflict.rdx, label %.preheader.preheader20, label %vector.ph

vector.ph:                                        ; preds = %vector.memcheck
  %n.vec = and i32 %25, -2
  %broadcast.splatinsert = insertelement <2 x double> poison, double %15, i64 0
  %broadcast.splat = shufflevector <2 x double> %broadcast.splatinsert, <2 x double> poison, <2 x i32> zeroinitializer
  br label %vector.body

vector.body:                                      ; preds = %vector.body, %vector.ph
  %index = phi i32 [ 0, %vector.ph ], [ %index.next, %vector.body ]
  %32 = sext i32 %index to i64
  %33 = getelementptr inbounds nuw [8 x i8], ptr %6, i64 %32
  %wide.load = load <2 x double>, ptr %33, align 8
  %34 = getelementptr inbounds nuw [8 x i8], ptr %1, i64 %32
  %wide.load19 = load <2 x double>, ptr %34, align 8
  %35 = fmul <2 x double> %broadcast.splat, %wide.load19
  %36 = fadd <2 x double> %wide.load, %35
  %37 = fmul <2 x double> %broadcast.splat, %36
  %38 = fadd <2 x double> %wide.load, %37
  %39 = fmul <2 x double> %broadcast.splat, %38
  %40 = fadd <2 x double> %wide.load, %39
  %41 = fmul <2 x double> %broadcast.splat, %40
  %42 = fadd <2 x double> %wide.load, %41
  %43 = fmul <2 x double> %broadcast.splat, %42
  %44 = fadd <2 x double> %wide.load, %43
  %45 = fmul <2 x double> %broadcast.splat, %44
  %46 = fadd <2 x double> %wide.load, %45
  %47 = fmul <2 x double> %broadcast.splat, %46
  %48 = fadd <2 x double> %wide.load, %47
  %49 = fmul <2 x double> %broadcast.splat, %48
  %50 = fadd <2 x double> %wide.load, %49
  %51 = fmul <2 x double> %broadcast.splat, %50
  %52 = fadd <2 x double> %wide.load, %51
  %53 = fmul <2 x double> %broadcast.splat, %52
  %54 = fadd <2 x double> %wide.load, %53
  %55 = fmul <2 x double> %broadcast.splat, %54
  %56 = fadd <2 x double> %wide.load, %55
  %57 = fmul <2 x double> %broadcast.splat, %56
  %58 = fadd <2 x double> %wide.load, %57
  %59 = fmul <2 x double> %broadcast.splat, %58
  %60 = fadd <2 x double> %wide.load, %59
  %61 = fmul <2 x double> %broadcast.splat, %60
  %62 = fadd <2 x double> %wide.load, %61
  %63 = fmul <2 x double> %broadcast.splat, %62
  %64 = fadd <2 x double> %wide.load, %63
  %65 = fmul <2 x double> %broadcast.splat, %64
  %66 = fadd <2 x double> %wide.load, %65
  %67 = fmul <2 x double> %broadcast.splat, %66
  %68 = fadd <2 x double> %wide.load, %67
  %69 = fmul <2 x double> %broadcast.splat, %68
  %70 = fadd <2 x double> %wide.load, %69
  %71 = fmul <2 x double> %broadcast.splat, %70
  %72 = fadd <2 x double> %wide.load, %71
  %73 = fmul <2 x double> %broadcast.splat, %72
  %74 = fadd <2 x double> %wide.load, %73
  %75 = fmul <2 x double> %broadcast.splat, %74
  %76 = fadd <2 x double> %wide.load, %75
  %77 = fmul <2 x double> %broadcast.splat, %76
  %78 = fadd <2 x double> %wide.load, %77
  %79 = fmul <2 x double> %broadcast.splat, %78
  %80 = fadd <2 x double> %wide.load, %79
  %81 = fmul <2 x double> %broadcast.splat, %80
  %82 = fadd <2 x double> %wide.load, %81
  %83 = fmul <2 x double> %broadcast.splat, %82
  %84 = fadd <2 x double> %wide.load, %83
  %85 = fmul <2 x double> %broadcast.splat, %84
  %86 = fadd <2 x double> %wide.load, %85
  %87 = fmul <2 x double> %broadcast.splat, %86
  %88 = fadd <2 x double> %wide.load, %87
  %89 = fmul <2 x double> %broadcast.splat, %88
  %90 = fadd <2 x double> %wide.load, %89
  %91 = fmul <2 x double> %broadcast.splat, %90
  %92 = fadd <2 x double> %wide.load, %91
  %93 = fmul <2 x double> %broadcast.splat, %92
  %94 = fadd <2 x double> %wide.load, %93
  %95 = fmul <2 x double> %broadcast.splat, %94
  %96 = fadd <2 x double> %wide.load, %95
  %97 = fmul <2 x double> %broadcast.splat, %96
  %98 = fadd <2 x double> %wide.load, %97
  %99 = getelementptr inbounds nuw [8 x i8], ptr %11, i64 %32
  store <2 x double> %98, ptr %99, align 8
  %index.next = add nuw i32 %index, 2
  %100 = icmp eq i32 %index.next, %n.vec
  br i1 %100, label %.preheader.preheader20, label %vector.body, !llvm.loop !1

.preheader.preheader20:                           ; preds = %vector.body, %vector.memcheck, %vector.scevcheck, %.preheader.preheader
  %.0.ph = phi i32 [ 0, %vector.memcheck ], [ 0, %vector.scevcheck ], [ 0, %.preheader.preheader ], [ %n.vec, %vector.body ]
  br label %.preheader

.preheader:                                       ; preds = %.preheader.preheader20, %109
  %.0 = phi i32 [ %179, %109 ], [ %.0.ph, %.preheader.preheader20 ]
  %101 = sext i32 %.0 to i64
  %102 = icmp sgt i32 %.0, -1
  %103 = icmp sgt i64 %3, %101
  %104 = and i1 %102, %103
  br i1 %104, label %105, label %180

105:                                              ; preds = %.preheader
  %106 = icmp sgt i64 %.fr, %101
  br i1 %106, label %107, label %181

107:                                              ; preds = %105
  %108 = icmp sgt i64 %13, %101
  br i1 %108, label %109, label %182

109:                                              ; preds = %107
  %110 = getelementptr inbounds nuw [8 x i8], ptr %6, i64 %101
  %111 = load double, ptr %110, align 8
  %112 = getelementptr inbounds nuw [8 x i8], ptr %1, i64 %101
  %113 = load double, ptr %112, align 8
  %114 = fmul double %15, %113
  %115 = fadd double %111, %114
  %116 = fmul double %15, %115
  %117 = fadd double %111, %116
  %118 = fmul double %15, %117
  %119 = fadd double %111, %118
  %120 = fmul double %15, %119
  %121 = fadd double %111, %120
  %122 = fmul double %15, %121
  %123 = fadd double %111, %122
  %124 = fmul double %15, %123
  %125 = fadd double %111, %124
  %126 = fmul double %15, %125
  %127 = fadd double %111, %126
  %128 = fmul double %15, %127
  %129 = fadd double %111, %128
  %130 = fmul double %15, %129
  %131 = fadd double %111, %130
  %132 = fmul double %15, %131
  %133 = fadd double %111, %132
  %134 = fmul double %15, %133
  %135 = fadd double %111, %134
  %136 = fmul double %15, %135
  %137 = fadd double %111, %136
  %138 = fmul double %15, %137
  %139 = fadd double %111, %138
  %140 = fmul double %15, %139
  %141 = fadd double %111, %140
  %142 = fmul double %15, %141
  %143 = fadd double %111, %142
  %144 = fmul double %15, %143
  %145 = fadd double %111, %144
  %146 = fmul double %15, %145
  %147 = fadd double %111, %146
  %148 = fmul double %15, %147
  %149 = fadd double %111, %148
  %150 = fmul double %15, %149
  %151 = fadd double %111, %150
  %152 = fmul double %15, %151
  %153 = fadd double %111, %152
  %154 = fmul double %15, %153
  %155 = fadd double %111, %154
  %156 = fmul double %15, %155
  %157 = fadd double %111, %156
  %158 = fmul double %15, %157
  %159 = fadd double %111, %158
  %160 = fmul double %15, %159
  %161 = fadd double %111, %160
  %162 = fmul double %15, %161
  %163 = fadd double %111, %162
  %164 = fmul double %15, %163
  %165 = fadd double %111, %164
  %166 = fmul double %15, %165
  %167 = fadd double %111, %166
  %168 = fmul double %15, %167
  %169 = fadd double %111, %168
  %170 = fmul double %15, %169
  %171 = fadd double %111, %170
  %172 = fmul double %15, %171
  %173 = fadd double %111, %172
  %174 = fmul double %15, %173
  %175 = fadd double %111, %174
  %176 = fmul double %15, %175
  %177 = fadd double %111, %176
  %178 = getelementptr inbounds nuw [8 x i8], ptr %11, i64 %101
  store double %177, ptr %178, align 8
  %179 = add nuw i32 %.0, 1
  %exitcond.not = icmp eq i32 %179, %20
  br i1 %exitcond.not, label %.loopexit, label %.preheader, !llvm.loop !4

.loopexit:                                        ; preds = %109, %16
  ret double 0.000000e+00

180:                                              ; preds = %.preheader
  tail call void @puts(ptr nonnull @assert_msg)
  tail call void @abort()
  unreachable

181:                                              ; preds = %105
  tail call void @puts(ptr nonnull @assert_msg_0)
  tail call void @abort()
  unreachable

182:                                              ; preds = %107
  tail call void @puts(ptr nonnull @assert_msg_1)
  tail call void @abort()
  unreachable
}

define noundef double @_mlir_ciface_vector(ptr nofree readonly captures(none) %0, ptr nofree readonly captures(none) %1, ptr nofree readonly captures(none) %2, double %3) local_unnamed_addr {
  %.elt1 = getelementptr inbounds nuw i8, ptr %0, i64 8
  %.unpack2 = load ptr, ptr %.elt1, align 8
  %.elt5 = getelementptr inbounds nuw i8, ptr %0, i64 24
  %.unpack6.unpack = load i64, ptr %.elt5, align 8
  %.elt12 = getelementptr inbounds nuw i8, ptr %1, i64 8
  %.unpack13 = load ptr, ptr %.elt12, align 8
  %.elt16 = getelementptr inbounds nuw i8, ptr %1, i64 24
  %.unpack17.unpack = load i64, ptr %.elt16, align 8
  %.elt23 = getelementptr inbounds nuw i8, ptr %2, i64 8
  %.unpack24 = load ptr, ptr %.elt23, align 8
  %.elt27 = getelementptr inbounds nuw i8, ptr %2, i64 24
  %.unpack28.unpack = load i64, ptr %.elt27, align 8
  %5 = tail call double @vector(ptr poison, ptr %.unpack2, i64 poison, i64 %.unpack6.unpack, i64 poison, ptr poison, ptr %.unpack13, i64 poison, i64 %.unpack17.unpack, i64 poison, ptr poison, ptr %.unpack24, i64 poison, i64 %.unpack28.unpack, i64 poison, double %3)
  ret double 0.000000e+00
}

declare i32 @krt_acc_map(ptr, ptr, ptr, ptr, double, ptr) local_unnamed_addr

define i32 @kacc_vector(ptr %0, ptr %1, ptr %2, ptr %3, double %4) local_unnamed_addr {
entry:
  %5 = tail call i32 @krt_acc_map(ptr %0, ptr %1, ptr %2, ptr %3, double %4, ptr nonnull @element)
  ret i32 %5
}

; Function Attrs: nocallback nocreateundeforpoison nofree nosync nounwind speculatable willreturn memory(none)
declare i64 @llvm.smax.i64(i64, i64) #2

; Function Attrs: nocallback nocreateundeforpoison nofree nosync nounwind speculatable willreturn memory(none)
declare i64 @llvm.umin.i64(i64, i64) #2

attributes #0 = { cold nofree noreturn nounwind }
attributes #1 = { mustprogress nofree norecurse nosync nounwind willreturn memory(none) }
attributes #2 = { nocallback nocreateundeforpoison nofree nosync nounwind speculatable willreturn memory(none) }

!llvm.module.flags = !{!0}

!0 = !{i32 2, !"Debug Info Version", i32 3}
!1 = distinct !{!1, !2, !3}
!2 = !{!"llvm.loop.isvectorized", i32 1}
!3 = !{!"llvm.loop.unroll.runtime.disable"}
!4 = distinct !{!4, !2}
