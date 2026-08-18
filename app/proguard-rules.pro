# ARMBIO4 ProGuard Rules
-keepclasseswithmembernames class * {
    native <methods>;
}
-keep class jp.co.capcom.android.bio4.** { *; }
-dontwarn javax.annotation.**
