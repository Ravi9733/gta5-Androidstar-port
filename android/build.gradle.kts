plugins {
    // AGP 8.7.x is compatible with the repository's Gradle 8.9 CI runner
    // and compileSdk 35 configuration.
    id("com.android.application") version "8.7.3" apply false
    id("org.jetbrains.kotlin.android") version "2.0.21" apply false
}
