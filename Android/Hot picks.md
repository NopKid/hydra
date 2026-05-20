pm -> android  package manager

NB: Android randomizes APKs to prevent hard-coded path attack

Split APKs ->  Your phone downloads what it needs. For instance a Kenyan user get the following.

    base.apk                  → Core app code. The brain.
	split_config.arm64_v8a    → Native code for YOUR specific CPU architecture
	split_config.en           → English language resources only
	split_config.xhdpi        → Screen resolution specific assets

-> dangerous code lives in `base.apk` and `arm64_v8a`
		The `base.apk` -> has the Java logic - authentication, API calls, Business logic.
		While the `arm64_v8a` had compiled C/C++ native libraries - memory handling, cryptography low level operations etc.


