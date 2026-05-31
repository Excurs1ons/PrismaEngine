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
            jniLibs.srcDirs("src/main/jniLibs")
        }
    }
}

val engineRoot = file("../../../..").absoluteFile
val pt3dRoot = file("$engineRoot/projects/PathTracing3D").absoluteFile
val csRuntimeDir = file("$engineRoot/build/android-arm64-debug/PrismaEngine.Host/android-arm64/publish")

tasks.register<Copy>("copyCsRuntime") {
    from(csRuntimeDir) { exclude("*.so", "*.so.*") }
    into("src/main/assets/runtime")
    onlyIf { csRuntimeDir.exists() }
}

tasks.register<Copy>("copyEngineShaders") {
    // 根 assets/shaders/（PBR IBL、2D、Forward、Deferred、SSAO 等全平台 shader）
    from("$engineRoot/assets/shaders") {
        include("**/*.spv")
        into("shaders")
    }
    // resources/common/shaders/glsl/（water、particles 等子目录 shader）
    from("$engineRoot/resources/common/shaders/glsl") {
        include("**/*.spv")
        into("shaders")
    }
    into("src/main/assets")
}

tasks.register<Copy>("copyPt3dAssets") {
    // Copy scenes and materials from PT3D
    from("$pt3dRoot/assets") {
        include("scenes/**")
        include("materials/**")
        include("models/**")
        include("PathTracing3D.jsonc")
    }
    // Copy shaders from PT3D (already compiled to .spv)
    from("$pt3dRoot/assets/shaders") {
        include("*.spv")
        into("shaders")
    }
    into("src/main/assets")
}

tasks.named("preBuild") {
    dependsOn("copyCsRuntime")
    dependsOn("copyEngineShaders")
    dependsOn("copyPt3dAssets")
}

dependencies {
    implementation(libs.appcompat)
}
