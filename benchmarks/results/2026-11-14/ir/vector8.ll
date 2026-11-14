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
  ret double %19
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
  %min.iters.check = icmp samesign ult i64 %umin17, 6
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
  %51 = getelementptr inbounds nuw [8 x i8], ptr %11, i64 %32
  store <2 x double> %50, ptr %51, align 8
  %index.next = add nuw i32 %index, 2
  %52 = icmp eq i32 %index.next, %n.vec
  br i1 %52, label %.preheader.preheader20, label %vector.body, !llvm.loop !1

.preheader.preheader20:                           ; preds = %vector.body, %vector.memcheck, %vector.scevcheck, %.preheader.preheader
  %.0.ph = phi i32 [ 0, %vector.memcheck ], [ 0, %vector.scevcheck ], [ 0, %.preheader.preheader ], [ %n.vec, %vector.body ]
  br label %.preheader

.preheader:                                       ; preds = %.preheader.preheader20, %61
  %.0 = phi i32 [ %83, %61 ], [ %.0.ph, %.preheader.preheader20 ]
  %53 = sext i32 %.0 to i64
  %54 = icmp sgt i32 %.0, -1
  %55 = icmp sgt i64 %3, %53
  %56 = and i1 %54, %55
  br i1 %56, label %57, label %84

57:                                               ; preds = %.preheader
  %58 = icmp sgt i64 %.fr, %53
  br i1 %58, label %59, label %85

59:                                               ; preds = %57
  %60 = icmp sgt i64 %13, %53
  br i1 %60, label %61, label %86

61:                                               ; preds = %59
  %62 = getelementptr inbounds nuw [8 x i8], ptr %6, i64 %53
  %63 = load double, ptr %62, align 8
  %64 = getelementptr inbounds nuw [8 x i8], ptr %1, i64 %53
  %65 = load double, ptr %64, align 8
  %66 = fmul double %15, %65
  %67 = fadd double %63, %66
  %68 = fmul double %15, %67
  %69 = fadd double %63, %68
  %70 = fmul double %15, %69
  %71 = fadd double %63, %70
  %72 = fmul double %15, %71
  %73 = fadd double %63, %72
  %74 = fmul double %15, %73
  %75 = fadd double %63, %74
  %76 = fmul double %15, %75
  %77 = fadd double %63, %76
  %78 = fmul double %15, %77
  %79 = fadd double %63, %78
  %80 = fmul double %15, %79
  %81 = fadd double %63, %80
  %82 = getelementptr inbounds nuw [8 x i8], ptr %11, i64 %53
  store double %81, ptr %82, align 8
  %83 = add nuw i32 %.0, 1
  %exitcond.not = icmp eq i32 %83, %20
  br i1 %exitcond.not, label %.loopexit, label %.preheader, !llvm.loop !4

.loopexit:                                        ; preds = %61, %16
  ret double 0.000000e+00

84:                                               ; preds = %.preheader
  tail call void @puts(ptr nonnull @assert_msg)
  tail call void @abort()
  unreachable

85:                                               ; preds = %57
  tail call void @puts(ptr nonnull @assert_msg_0)
  tail call void @abort()
  unreachable

86:                                               ; preds = %59
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
