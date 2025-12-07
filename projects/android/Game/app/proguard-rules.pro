
#-------------------------------------------------------------------------------
# ProGuard规则 for SDL (Simple DirectMedia Layer)
#-------------------------------------------------------------------------------

# 保留SDLActivity类及其所有成员不被混淆和移除。
# 这是最关键的一步，因为原生代码通过JNI按名称查找这些方法。
-keep class org.libsdl.app.SDLActivity { *; }

# 为了安全起见，同样保留SDL相关的其他重要类。
-keep class org.libsdl.app.SDLAudioManager { *; }
-keep class org.libsdl.app.SDLSurface { *; }
-keep class org.libsdl.app.SDLGenericMotionListener_API14 { *; }
-keep class org.libsdl.app.SDLControllerManager { *; }
-keep class org.libsdl.app.HIDDeviceManager { *; }