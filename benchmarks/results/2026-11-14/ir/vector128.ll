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
  %68 = fmul double %2, %67
  %69 = fadd double %1, %68
  %70 = fmul double %2, %69
  %71 = fadd double %1, %70
  %72 = fmul double %2, %71
  %73 = fadd double %1, %72
  %74 = fmul double %2, %73
  %75 = fadd double %1, %74
  %76 = fmul double %2, %75
  %77 = fadd double %1, %76
  %78 = fmul double %2, %77
  %79 = fadd double %1, %78
  %80 = fmul double %2, %79
  %81 = fadd double %1, %80
  %82 = fmul double %2, %81
  %83 = fadd double %1, %82
  %84 = fmul double %2, %83
  %85 = fadd double %1, %84
  %86 = fmul double %2, %85
  %87 = fadd double %1, %86
  %88 = fmul double %2, %87
  %89 = fadd double %1, %88
  %90 = fmul double %2, %89
  %91 = fadd double %1, %90
  %92 = fmul double %2, %91
  %93 = fadd double %1, %92
  %94 = fmul double %2, %93
  %95 = fadd double %1, %94
  %96 = fmul double %2, %95
  %97 = fadd double %1, %96
  %98 = fmul double %2, %97
  %99 = fadd double %1, %98
  %100 = fmul double %2, %99
  %101 = fadd double %1, %100
  %102 = fmul double %2, %101
  %103 = fadd double %1, %102
  %104 = fmul double %2, %103
  %105 = fadd double %1, %104
  %106 = fmul double %2, %105
  %107 = fadd double %1, %106
  %108 = fmul double %2, %107
  %109 = fadd double %1, %108
  %110 = fmul double %2, %109
  %111 = fadd double %1, %110
  %112 = fmul double %2, %111
  %113 = fadd double %1, %112
  %114 = fmul double %2, %113
  %115 = fadd double %1, %114
  %116 = fmul double %2, %115
  %117 = fadd double %1, %116
  %118 = fmul double %2, %117
  %119 = fadd double %1, %118
  %120 = fmul double %2, %119
  %121 = fadd double %1, %120
  %122 = fmul double %2, %121
  %123 = fadd double %1, %122
  %124 = fmul double %2, %123
  %125 = fadd double %1, %124
  %126 = fmul double %2, %125
  %127 = fadd double %1, %126
  %128 = fmul double %2, %127
  %129 = fadd double %1, %128
  %130 = fmul double %2, %129
  %131 = fadd double %1, %130
  %132 = fmul double %2, %131
  %133 = fadd double %1, %132
  %134 = fmul double %2, %133
  %135 = fadd double %1, %134
  %136 = fmul double %2, %135
  %137 = fadd double %1, %136
  %138 = fmul double %2, %137
  %139 = fadd double %1, %138
  %140 = fmul double %2, %139
  %141 = fadd double %1, %140
  %142 = fmul double %2, %141
  %143 = fadd double %1, %142
  %144 = fmul double %2, %143
  %145 = fadd double %1, %144
  %146 = fmul double %2, %145
  %147 = fadd double %1, %146
  %148 = fmul double %2, %147
  %149 = fadd double %1, %148
  %150 = fmul double %2, %149
  %151 = fadd double %1, %150
  %152 = fmul double %2, %151
  %153 = fadd double %1, %152
  %154 = fmul double %2, %153
  %155 = fadd double %1, %154
  %156 = fmul double %2, %155
  %157 = fadd double %1, %156
  %158 = fmul double %2, %157
  %159 = fadd double %1, %158
  %160 = fmul double %2, %159
  %161 = fadd double %1, %160
  %162 = fmul double %2, %161
  %163 = fadd double %1, %162
  %164 = fmul double %2, %163
  %165 = fadd double %1, %164
  %166 = fmul double %2, %165
  %167 = fadd double %1, %166
  %168 = fmul double %2, %167
  %169 = fadd double %1, %168
  %170 = fmul double %2, %169
  %171 = fadd double %1, %170
  %172 = fmul double %2, %171
  %173 = fadd double %1, %172
  %174 = fmul double %2, %173
  %175 = fadd double %1, %174
  %176 = fmul double %2, %175
  %177 = fadd double %1, %176
  %178 = fmul double %2, %177
  %179 = fadd double %1, %178
  %180 = fmul double %2, %179
  %181 = fadd double %1, %180
  %182 = fmul double %2, %181
  %183 = fadd double %1, %182
  %184 = fmul double %2, %183
  %185 = fadd double %1, %184
  %186 = fmul double %2, %185
  %187 = fadd double %1, %186
  %188 = fmul double %2, %187
  %189 = fadd double %1, %188
  %190 = fmul double %2, %189
  %191 = fadd double %1, %190
  %192 = fmul double %2, %191
  %193 = fadd double %1, %192
  %194 = fmul double %2, %193
  %195 = fadd double %1, %194
  %196 = fmul double %2, %195
  %197 = fadd double %1, %196
  %198 = fmul double %2, %197
  %199 = fadd double %1, %198
  %200 = fmul double %2, %199
  %201 = fadd double %1, %200
  %202 = fmul double %2, %201
  %203 = fadd double %1, %202
  %204 = fmul double %2, %203
  %205 = fadd double %1, %204
  %206 = fmul double %2, %205
  %207 = fadd double %1, %206
  %208 = fmul double %2, %207
  %209 = fadd double %1, %208
  %210 = fmul double %2, %209
  %211 = fadd double %1, %210
  %212 = fmul double %2, %211
  %213 = fadd double %1, %212
  %214 = fmul double %2, %213
  %215 = fadd double %1, %214
  %216 = fmul double %2, %215
  %217 = fadd double %1, %216
  %218 = fmul double %2, %217
  %219 = fadd double %1, %218
  %220 = fmul double %2, %219
  %221 = fadd double %1, %220
  %222 = fmul double %2, %221
  %223 = fadd double %1, %222
  %224 = fmul double %2, %223
  %225 = fadd double %1, %224
  %226 = fmul double %2, %225
  %227 = fadd double %1, %226
  %228 = fmul double %2, %227
  %229 = fadd double %1, %228
  %230 = fmul double %2, %229
  %231 = fadd double %1, %230
  %232 = fmul double %2, %231
  %233 = fadd double %1, %232
  %234 = fmul double %2, %233
  %235 = fadd double %1, %234
  %236 = fmul double %2, %235
  %237 = fadd double %1, %236
  %238 = fmul double %2, %237
  %239 = fadd double %1, %238
  %240 = fmul double %2, %239
  %241 = fadd double %1, %240
  %242 = fmul double %2, %241
  %243 = fadd double %1, %242
  %244 = fmul double %2, %243
  %245 = fadd double %1, %244
  %246 = fmul double %2, %245
  %247 = fadd double %1, %246
  %248 = fmul double %2, %247
  %249 = fadd double %1, %248
  %250 = fmul double %2, %249
  %251 = fadd double %1, %250
  %252 = fmul double %2, %251
  %253 = fadd double %1, %252
  %254 = fmul double %2, %253
  %255 = fadd double %1, %254
  %256 = fmul double %2, %255
  %257 = fadd double %1, %256
  %258 = fmul double %2, %257
  %259 = fadd double %1, %258
  ret double %259
}

define noundef double @vector(ptr nofree readnone captures(none) %0, ptr nofree readonly captures(none) %1, i64 %2, i64 %3, i64 %4, ptr nofree readnone captures(none) %5, ptr nofree readonly captures(none) %6, i64 %7, i64 %8, i64 %9, ptr nofree readnone captures(none) %10, ptr nofree writeonly captures(none) %11, i64 %12, i64 %13, i64 %14, double %15) local_unnamed_addr {
  %17 = trunc i64 %3 to i32
  %18 = icmp sgt i32 %17, 0
  br i1 %18, label %.preheader, label %.loopexit

.preheader:                                       ; preds = %16, %27
  %.0 = phi i32 [ %34, %27 ], [ 0, %16 ]
  %19 = sext i32 %.0 to i64
  %20 = icmp sgt i32 %.0, -1
  %21 = icmp sgt i64 %3, %19
  %22 = and i1 %20, %21
  br i1 %22, label %23, label %35

23:                                               ; preds = %.preheader
  %24 = icmp sgt i64 %8, %19
  br i1 %24, label %25, label %36

25:                                               ; preds = %23
  %26 = icmp sgt i64 %13, %19
  br i1 %26, label %27, label %37

27:                                               ; preds = %25
  %28 = getelementptr inbounds nuw [8 x i8], ptr %1, i64 %19
  %29 = load double, ptr %28, align 8
  %30 = getelementptr inbounds nuw [8 x i8], ptr %6, i64 %19
  %31 = load double, ptr %30, align 8
  %32 = tail call double @element(double %29, double %31, double %15)
  %33 = getelementptr inbounds nuw [8 x i8], ptr %11, i64 %19
  store double %32, ptr %33, align 8
  %34 = add nuw i32 %.0, 1
  %exitcond.not = icmp eq i32 %34, %17
  br i1 %exitcond.not, label %.loopexit, label %.preheader

.loopexit:                                        ; preds = %27, %16
  ret double 0.000000e+00

35:                                               ; preds = %.preheader
  tail call void @puts(ptr nonnull @assert_msg)
  tail call void @abort()
  unreachable

36:                                               ; preds = %23
  tail call void @puts(ptr nonnull @assert_msg_0)
  tail call void @abort()
  unreachable

37:                                               ; preds = %25
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
  %5 = trunc i64 %.unpack6.unpack to i32
  %6 = icmp sgt i32 %5, 0
  br i1 %6, label %.preheader.i, label %vector.exit

.preheader.i:                                     ; preds = %4, %15
  %.0.i = phi i32 [ %22, %15 ], [ 0, %4 ]
  %7 = sext i32 %.0.i to i64
  %8 = icmp sgt i32 %.0.i, -1
  %9 = icmp sgt i64 %.unpack6.unpack, %7
  %10 = and i1 %8, %9
  br i1 %10, label %11, label %23

11:                                               ; preds = %.preheader.i
  %12 = icmp sgt i64 %.unpack17.unpack, %7
  br i1 %12, label %13, label %24

13:                                               ; preds = %11
  %14 = icmp sgt i64 %.unpack28.unpack, %7
  br i1 %14, label %15, label %25

15:                                               ; preds = %13
  %16 = getelementptr inbounds nuw [8 x i8], ptr %.unpack2, i64 %7
  %17 = load double, ptr %16, align 8
  %18 = getelementptr inbounds nuw [8 x i8], ptr %.unpack13, i64 %7
  %19 = load double, ptr %18, align 8
  %20 = tail call double @element(double %17, double %19, double %3)
  %21 = getelementptr inbounds nuw [8 x i8], ptr %.unpack24, i64 %7
  store double %20, ptr %21, align 8
  %22 = add nuw i32 %.0.i, 1
  %exitcond.not.i = icmp eq i32 %22, %5
  br i1 %exitcond.not.i, label %vector.exit, label %.preheader.i

23:                                               ; preds = %.preheader.i
  tail call void @puts(ptr nonnull @assert_msg)
  tail call void @abort()
  unreachable

24:                                               ; preds = %11
  tail call void @puts(ptr nonnull @assert_msg_0)
  tail call void @abort()
  unreachable

25:                                               ; preds = %13
  tail call void @puts(ptr nonnull @assert_msg_1)
  tail call void @abort()
  unreachable

vector.exit:                                      ; preds = %15, %4
  ret double 0.000000e+00
}

declare i32 @krt_acc_map(ptr, ptr, ptr, ptr, double, ptr) local_unnamed_addr

define i32 @kacc_vector(ptr %0, ptr %1, ptr %2, ptr %3, double %4) local_unnamed_addr {
entry:
  %5 = tail call i32 @krt_acc_map(ptr %0, ptr %1, ptr %2, ptr %3, double %4, ptr nonnull @element)
  ret i32 %5
}

attributes #0 = { cold nofree noreturn nounwind }
attributes #1 = { mustprogress nofree norecurse nosync nounwind willreturn memory(none) }

!llvm.module.flags = !{!0}

!0 = !{i32 2, !"Debug Info Version", i32 3}
