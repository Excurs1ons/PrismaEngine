plugins {
    alias(libs.plugins.android.application)
}

android {
    namespace = "com.example.myapplication"
    compileSdk = 34

    defaultConfig {
        applicationId = "com.example.myapplication"
        minSdk = 30
        targetSdk = 34
        versionCode = 1
        versionName = "1.0"

        testInstrumentationRunner = "androidx.test.runner.AndroidJUnitRunner"
        externalNativeBuild {
            cmake {
                cppFlags += "-std=c++20"
                cppFlags += "-DPRISMA_ENABLE_RENDER_VULKAN"
                abiFilters("arm64-v8a")
            }
        }
        ndk {
            abiFilters.add("arm64-v8a")
        }
    }

    buildTypes {
        release {
            isMinifyEnabled = true
            isShrinkResources = true
            proguardFiles(
                getDefaultProguardFile("proguard-android-optimize.txt"),
                "proguard-rules.pro"
            )
            signingConfig = signingConfigs.getByName("debug")
        }
    }
    compileOptions {
        sourceCompatibility = JavaVersion.VERSION_11
        targetCompatibility = JavaVersion.VERSION_11
    }
    buildFeatures {
        prefab = true
        // 启用着色器编译（将 .vert/.frag 编译为 .spv）
        shaders = true
    }
    externalNativeBuild {
        cmake {
            path = file("src/main/cpp/CMakeLists.txt")
            version = "3.22.1"
        }
    }
}

// ========== 复制引擎资源到 assets ==========

// 获取引擎根目录（从 app 向上 6 级到 PrismaEngine 根目录）
val engineRoot = file("../../..").absoluteFile

// 复制 Android runtime 资源
tasks.register<Copy>("copyEngineRuntimeAssets") {
    description = "复制引擎资源到 assets"
    group = "internal"

    // 复制 Android 着色器
    from("$engineRoot/resources/android/shaders") {
        into("shaders")
    }
    // 复制 Android 纹理
    from("$engineRoot/resources/android/textures") {
        into("textures")
    }

    into("src/main/assets")

    // 忽略空目录
    includeEmptyDirs = false
}

// 在预构建时执行复制
tasks.named("preBuild") {
    dependsOn("copyEngineRuntimeAssets")
}

dependencies {

    implementation(libs.appcompat)
    implementation(libs.material)
    implementation(libs.games.activity)
    testImplementation(libs.junit)
    androidTestImplementation(libs.ext.junit)
    androidTestImplementation(libs.espresso.core)
}