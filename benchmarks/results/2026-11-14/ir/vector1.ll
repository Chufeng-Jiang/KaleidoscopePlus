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
  ret double %5
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
  %26 = add nuw i32 %25, 1
  %min.iters.check = icmp samesign ult i64 %umin17, 12
  br i1 %min.iters.check, label %.preheader.preheader22, label %vector.scevcheck

vector.scevcheck:                                 ; preds = %.preheader.preheader
  %smax = tail call i64 @llvm.smax.i64(i64 %13, i64 0)
  %27 = add i64 %3, 4294967295
  %28 = and i64 %27, 4294967295
  %umin = tail call i64 @llvm.umin.i64(i64 %smax, i64 %28)
  %29 = freeze i64 %umin
  %smax6 = tail call i64 @llvm.smax.i64(i64 %.fr, i64 0)
  %umin7 = tail call i64 @llvm.umin.i64(i64 %29, i64 %smax6)
  %smax8 = tail call i64 @llvm.smax.i64(i64 %3, i64 0)
  %umin9 = tail call i64 @llvm.umin.i64(i64 %smax8, i64 %umin7)
  %30 = icmp samesign ugt i64 %umin9, 2147483647
  br i1 %30, label %.preheader.preheader22, label %vector.memcheck

vector.memcheck:                                  ; preds = %vector.scevcheck
  %31 = sub i64 %18, %19
  %diff.check = icmp ugt i64 %31, -32
  %32 = sub i64 %17, %19
  %diff.check11 = icmp ugt i64 %32, -32
  %conflict.rdx = or i1 %diff.check, %diff.check11
  br i1 %conflict.rdx, label %.preheader.preheader22, label %vector.ph

vector.ph:                                        ; preds = %vector.memcheck
  %n.mod.vf = and i32 %26, 3
  %33 = icmp eq i32 %n.mod.vf, 0
  %34 = select i1 %33, i32 4, i32 %n.mod.vf
  %n.vec = sub i32 %26, %34
  %broadcast.splatinsert = insertelement <2 x double> poison, double %15, i64 0
  %broadcast.splat = shufflevector <2 x double> %broadcast.splatinsert, <2 x double> poison, <2 x i32> zeroinitializer
  br label %vector.body

vector.body:                                      ; preds = %vector.body, %vector.ph
  %index = phi i32 [ 0, %vector.ph ], [ %index.next, %vector.body ]
  %35 = sext i32 %index to i64
  %36 = getelementptr inbounds nuw [8 x i8], ptr %6, i64 %35
  %37 = getelementptr inbounds nuw i8, ptr %36, i64 16
  %wide.load = load <2 x double>, ptr %36, align 8
  %wide.load19 = load <2 x double>, ptr %37, align 8
  %38 = getelementptr inbounds nuw [8 x i8], ptr %1, i64 %35
  %39 = getelementptr inbounds nuw i8, ptr %38, i64 16
  %wide.load20 = load <2 x double>, ptr %38, align 8
  %wide.load21 = load <2 x double>, ptr %39, align 8
  %40 = fmul <2 x double> %broadcast.splat, %wide.load20
  %41 = fmul <2 x double> %broadcast.splat, %wide.load21
  %42 = fadd <2 x double> %wide.load, %40
  %43 = fadd <2 x double> %wide.load19, %41
  %44 = getelementptr inbounds nuw [8 x i8], ptr %11, i64 %35
  %45 = getelementptr inbounds nuw i8, ptr %44, i64 16
  store <2 x double> %42, ptr %44, align 8
  store <2 x double> %43, ptr %45, align 8
  %index.next = add nuw i32 %index, 4
  %46 = icmp eq i32 %index.next, %n.vec
  br i1 %46, label %.preheader.preheader22, label %vector.body, !llvm.loop !1

.preheader.preheader22:                           ; preds = %vector.body, %vector.memcheck, %vector.scevcheck, %.preheader.preheader
  %.0.ph = phi i32 [ 0, %vector.memcheck ], [ 0, %vector.scevcheck ], [ 0, %.preheader.preheader ], [ %n.vec, %vector.body ]
  br label %.preheader

.preheader:                                       ; preds = %.preheader.preheader22, %55
  %.0 = phi i32 [ %63, %55 ], [ %.0.ph, %.preheader.preheader22 ]
  %47 = sext i32 %.0 to i64
  %48 = icmp sgt i32 %.0, -1
  %49 = icmp sgt i64 %3, %47
  %50 = and i1 %48, %49
  br i1 %50, label %51, label %64

51:                                               ; preds = %.preheader
  %52 = icmp sgt i64 %.fr, %47
  br i1 %52, label %53, label %65

53:                                               ; preds = %51
  %54 = icmp sgt i64 %13, %47
  br i1 %54, label %55, label %66

55:                                               ; preds = %53
  %56 = getelementptr inbounds nuw [8 x i8], ptr %6, i64 %47
  %57 = load double, ptr %56, align 8
  %58 = getelementptr inbounds nuw [8 x i8], ptr %1, i64 %47
  %59 = load double, ptr %58, align 8
  %60 = fmul double %15, %59
  %61 = fadd double %57, %60
  %62 = getelementptr inbounds nuw [8 x i8], ptr %11, i64 %47
  store double %61, ptr %62, align 8
  %63 = add nuw i32 %.0, 1
  %exitcond.not = icmp eq i32 %63, %20
  br i1 %exitcond.not, label %.loopexit, label %.preheader, !llvm.loop !4

.loopexit:                                        ; preds = %55, %16
  ret double 0.000000e+00

64:                                               ; preds = %.preheader
  tail call void @puts(ptr nonnull @assert_msg)
  tail call void @abort()
  unreachable

65:                                               ; preds = %51
  tail call void @puts(ptr nonnull @assert_msg_0)
  tail call void @abort()
  unreachable

66:                                               ; preds = %53
  tail call void @puts(ptr nonnull @assert_msg_1)
  tail call void @abort()
  unreachable
}

define noundef double @_mlir_ciface_vector(ptr nofree readonly captures(none) %0, ptr nofree readonly captures(none) %1, ptr nofree readonly captures(none) %2, double %3) local_unnamed_addr {
  %.elt1 = getelementptr inbounds nuw i8, ptr %0, i64 8
  %.unpack2 = load ptr, ptr %.elt1, align 8
  %.unpack242 = ptrtoaddr ptr %.unpack2 to i64
  %.elt5 = getelementptr inbounds nuw i8, ptr %0, i64 24
  %.unpack6.unpack = load i64, ptr %.elt5, align 8
  %.elt12 = getelementptr inbounds nuw i8, ptr %1, i64 8
  %.unpack13 = load ptr, ptr %.elt12, align 8
  %.unpack1341 = ptrtoaddr ptr %.unpack13 to i64
  %.elt16 = getelementptr inbounds nuw i8, ptr %1, i64 24
  %.unpack17.unpack = load i64, ptr %.elt16, align 8
  %.unpack17.unpack.fr = freeze i64 %.unpack17.unpack
  %.elt23 = getelementptr inbounds nuw i8, ptr %2, i64 8
  %.unpack24 = load ptr, ptr %.elt23, align 8
  %.unpack2440 = ptrtoaddr ptr %.unpack24 to i64
  %.elt27 = getelementptr inbounds nuw i8, ptr %2, i64 24
  %.unpack28.unpack = load i64, ptr %.elt27, align 8
  %5 = trunc i64 %.unpack6.unpack to i32
  %6 = icmp sgt i32 %5, 0
  br i1 %6, label %.preheader.i.preheader, label %vector.exit

.preheader.i.preheader:                           ; preds = %4
  %smax44 = tail call i64 @llvm.smax.i64(i64 %.unpack28.unpack, i64 0)
  %7 = add i64 %.unpack6.unpack, 4294967295
  %8 = and i64 %7, 4294967295
  %umin45 = tail call i64 @llvm.umin.i64(i64 %smax44, i64 %8)
  %9 = freeze i64 %umin45
  %smax46 = tail call i64 @llvm.smax.i64(i64 %.unpack17.unpack.fr, i64 0)
  %umin47 = tail call i64 @llvm.umin.i64(i64 %9, i64 %smax46)
  %smax48 = tail call i64 @llvm.smax.i64(i64 %.unpack6.unpack, i64 0)
  %umin49 = tail call i64 @llvm.umin.i64(i64 %smax48, i64 %umin47)
  %umin50 = tail call i64 @llvm.umin.i64(i64 %umin49, i64 2147483648)
  %10 = trunc nuw i64 %umin50 to i32
  %11 = add nuw i32 %10, 1
  %min.iters.check = icmp samesign ult i64 %umin49, 12
  br i1 %min.iters.check, label %.preheader.i.preheader54, label %vector.scevcheck

vector.scevcheck:                                 ; preds = %.preheader.i.preheader
  %smax = tail call i64 @llvm.smax.i64(i64 %.unpack28.unpack, i64 0)
  %12 = add i64 %.unpack6.unpack, 4294967295
  %13 = and i64 %12, 4294967295
  %umin = tail call i64 @llvm.umin.i64(i64 %smax, i64 %13)
  %14 = freeze i64 %umin
  %smax35 = tail call i64 @llvm.smax.i64(i64 %.unpack17.unpack.fr, i64 0)
  %umin36 = tail call i64 @llvm.umin.i64(i64 %14, i64 %smax35)
  %smax37 = tail call i64 @llvm.smax.i64(i64 %.unpack6.unpack, i64 0)
  %umin38 = tail call i64 @llvm.umin.i64(i64 %smax37, i64 %umin36)
  %15 = icmp samesign ugt i64 %umin38, 2147483647
  br i1 %15, label %.preheader.i.preheader54, label %vector.memcheck

vector.memcheck:                                  ; preds = %vector.scevcheck
  %16 = sub i64 %.unpack1341, %.unpack2440
  %diff.check = icmp ugt i64 %16, -32
  %17 = sub i64 %.unpack242, %.unpack2440
  %diff.check43 = icmp ugt i64 %17, -32
  %conflict.rdx = or i1 %diff.check, %diff.check43
  br i1 %conflict.rdx, label %.preheader.i.preheader54, label %vector.ph

vector.ph:                                        ; preds = %vector.memcheck
  %n.mod.vf = and i32 %11, 3
  %18 = icmp eq i32 %n.mod.vf, 0
  %19 = select i1 %18, i32 4, i32 %n.mod.vf
  %n.vec = sub i32 %11, %19
  %broadcast.splatinsert = insertelement <2 x double> poison, double %3, i64 0
  %broadcast.splat = shufflevector <2 x double> %broadcast.splatinsert, <2 x double> poison, <2 x i32> zeroinitializer
  br label %vector.body

vector.body:                                      ; preds = %vector.body, %vector.ph
  %index = phi i32 [ 0, %vector.ph ], [ %index.next, %vector.body ]
  %20 = sext i32 %index to i64
  %21 = getelementptr inbounds nuw [8 x i8], ptr %.unpack13, i64 %20
  %22 = getelementptr inbounds nuw i8, ptr %21, i64 16
  %wide.load = load <2 x double>, ptr %21, align 8
  %wide.load51 = load <2 x double>, ptr %22, align 8
  %23 = getelementptr inbounds nuw [8 x i8], ptr %.unpack2, i64 %20
  %24 = getelementptr inbounds nuw i8, ptr %23, i64 16
  %wide.load52 = load <2 x double>, ptr %23, align 8
  %wide.load53 = load <2 x double>, ptr %24, align 8
  %25 = fmul <2 x double> %broadcast.splat, %wide.load52
  %26 = fmul <2 x double> %broadcast.splat, %wide.load53
  %27 = fadd <2 x double> %wide.load, %25
  %28 = fadd <2 x double> %wide.load51, %26
  %29 = getelementptr inbounds nuw [8 x i8], ptr %.unpack24, i64 %20
  %30 = getelementptr inbounds nuw i8, ptr %29, i64 16
  store <2 x double> %27, ptr %29, align 8
  store <2 x double> %28, ptr %30, align 8
  %index.next = add nuw i32 %index, 4
  %31 = icmp eq i32 %index.next, %n.vec
  br i1 %31, label %.preheader.i.preheader54, label %vector.body, !llvm.loop !5

.preheader.i.preheader54:                         ; preds = %vector.body, %vector.memcheck, %vector.scevcheck, %.preheader.i.preheader
  %.0.i.ph = phi i32 [ 0, %vector.memcheck ], [ 0, %vector.scevcheck ], [ 0, %.preheader.i.preheader ], [ %n.vec, %vector.body ]
  br label %.preheader.i

.preheader.i:                                     ; preds = %.preheader.i.preheader54, %40
  %.0.i = phi i32 [ %48, %40 ], [ %.0.i.ph, %.preheader.i.preheader54 ]
  %32 = sext i32 %.0.i to i64
  %33 = icmp sgt i32 %.0.i, -1
  %34 = icmp sgt i64 %.unpack6.unpack, %32
  %35 = and i1 %33, %34
  br i1 %35, label %36, label %49

36:                                               ; preds = %.preheader.i
  %37 = icmp sgt i64 %.unpack17.unpack.fr, %32
  br i1 %37, label %38, label %50

38:                                               ; preds = %36
  %39 = icmp sgt i64 %.unpack28.unpack, %32
  br i1 %39, label %40, label %51

40:                                               ; preds = %38
  %41 = getelementptr inbounds nuw [8 x i8], ptr %.unpack13, i64 %32
  %42 = load double, ptr %41, align 8
  %43 = getelementptr inbounds nuw [8 x i8], ptr %.unpack2, i64 %32
  %44 = load double, ptr %43, align 8
  %45 = fmul double %3, %44
  %46 = fadd double %42, %45
  %47 = getelementptr inbounds nuw [8 x i8], ptr %.unpack24, i64 %32
  store double %46, ptr %47, align 8
  %48 = add nuw i32 %.0.i, 1
  %exitcond.not.i = icmp eq i32 %48, %5
  br i1 %exitcond.not.i, label %vector.exit, label %.preheader.i, !llvm.loop !6

49:                                               ; preds = %.preheader.i
  tail call void @puts(ptr nonnull @assert_msg)
  tail call void @abort()
  unreachable

50:                                               ; preds = %36
  tail call void @puts(ptr nonnull @assert_msg_0)
  tail call void @abort()
  unreachable

51:                                               ; preds = %38
  tail call void @puts(ptr nonnull @assert_msg_1)
  tail call void @abort()
  unreachable

vector.exit:                                      ; preds = %40, %4
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
!5 = distinct !{!5, !2, !3}
!6 = distinct !{!6, !2}
