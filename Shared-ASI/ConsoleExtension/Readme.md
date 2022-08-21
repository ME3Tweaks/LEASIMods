The Console extension ASI adds new console commands to Mass Effect 1/2/3 Legendary Edition

Commands added by this ASI:
* `savecam`: saves the position, yaw, and pitch of the camera to one of 100 slots. (Roll and FOV are not saved!) 
Must be followed by an integer, 0-99. Example usage: `savecam 0`
* `loadcam`: same as `savecam`, but sets the camera to a saved position. Should be used while in flycam/photo mode. (If in photo mode, the camera may be snapped back to a position within the restricted area. I suggest using a mod that removes photo mode restrictions for best effect)

Saved positions persist between sessions in a file within the game install: "Mass Effect Legendary Edition\Game\ME{1|2|3}\Binaries\Win64\savedCams" 
