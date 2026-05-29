plugins {
    alias(libs.plugins.android.application)
}

android {
    namespace = "com.prismaengine"
    compileSdk = 34

    defaultConfig {
        applicationId = "com.prismaengine.android"
        minSdk = 26
        targetSdk = 34
        versionCode = 1
        versionName = "1.0.0"
        ndk { abiFilters.add("arm64-v8a") }
    }

    buildTypes {
        release {
            isMinifyEnabled = false
            proguardFiles(getDefaultProguardFile("proguard-android-optimize.txt"), "proguard-rules.pro")
        }
    }

    sourceSets {
        getByName("main") {
            jniLibs.srcDirs("jniLibs")
        }
    }
}

val engineRoot = file("../../../..").absoluteFile
val csRuntimeDir = file("$engineRoot/build/android-arm64-debug/PrismaEngine.Host/android-arm64/publish")

tasks.register<Copy>("copyCsRuntime") {
    from(csRuntimeDir) { exclude("*.so", "*.so.*") }
    into("src/main/assets/runtime")
    onlyIf { csRuntimeDir.exists() }
}

tasks.register<Copy>("copyShaders") {
    from("$engineRoot/resources/common/shaders/glsl") { into("shaders") }
    into("src/main/assets")
}

tasks.named("preBuild") {
    dependsOn("copyCsRuntime")
    dependsOn("copyShaders")
}

dependencies {
    implementation(libs.appcompat)
}
