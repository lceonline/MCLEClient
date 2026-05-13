# Approach to Backported Features
All backported features incorperated into MCLEMP should be, when merged, functionally identical to their state in the version of the game we're currently targeting. This should be in reference to a known 4J build of LCE. Verification can either be done by doing a decompilation based match of the implementation or, alternatively, all functionality and limitations of the given feature should be compared against the version of LCE we're targeting.

# Approach to Downported Features
All downported features incorporated into MCLEMP should be, when merged, functionally identical to their state in the version of the game they originated from. This should be in reference to a known 4J build of LCE. erification can either be done by doing a decompilation based match of the implementation against the source build or, alternatively, all functionality and limitations of the given feature should be compared against the newer LCE version it was taken from.

# Approach to Bugfixes
Anything that does not behave in an "expected" manner, especially if its behavior is not widely accepted as a gameplay mechanic, is valid for fixing in our repository. This includes bugfixes that were made in versions past the version we target, but excludes any visual changes that may not have been included at the build we're targeting.

If you provide a visual bugfix that fixes a distinctive quirk of the LCE renderer, it should be provided in an "off by default" state that can be toggled on in-game by the user. There is no guarantee that we will merge it.

If your visual bugfix is a fix added in a future version of LCE than the one we're targeting, it should also be put behind a toggle or equivalent system for keeping it off by default.

# Targeted Version
We are targeting to keep this on TU19 with the exclusion of neoLegacy for Higher TU's, If there are any features for versions maintained by [neoLegacy](https://github.com/neoStudiosLCE/neoLegacy) that are missing make a PR to there repository and it will be brought to this, we also allow PR's that Downport to get older TU's.
