# Grind Rails
Brings rail grinding into SMG2!<br>
Requires a Path to work, and its recommended to be paired with a collisionless visual model/effect.

## Obj_Args
| Arg | Name | Description | Default
|---|---|---|---
| 0 | Snap Radius | How far the player needs to be from the rail to start grinding, times 1000.<br> (i.e a value of 250,000 = 250.000) | 100,000 (100.000)
| 1 | Jump Behavior at End | When reaching the rail's end, how should the player detatch?<br> (Bit 0: Jump, Bit 1: Allow Left-Right Influence) | 1
| 2 | Reattatch Delay | After leaving the rail, how long (in frames) until the player can reattatch to the rail? | 30
| 3 | SW_B Behavior | When should SW_B Activate?<br> (0: While riding rail, 1: When reaching the rail's end) | 1
| 4 | Collision Behavior | How should the player react when hitting a surface while grinding?<br>(0: Damage, 1: Death) | 0

## Point_Args
| Arg | Name | Description | Default
|---|---|---|---
| 0 | Speed | How fast should the player grind along the rail, times 1000.<br> (i.e a value of 15,000 = 15.000) | 20,000<br>(20.000)
| 1 | Acceleration | How fast should the player accel/deccel to reach the target speed, times 1000. | 1,000<br>(1.000)
| 2 | Momentum Type | Should the rail impact the player's jump height in addition to speed?<br>(0: No, 1: Yes) | 0
| 3 | Momentum Influence | What portion of the player's rail speed should go into jumping, times 1000? | 1,000<br>(1.000)
| 4 | Left-Right Influence | How strongly should the player be able to jump left/right, times 1000? | 5,000<br>(5.000)
| 5 | Allow Jumping | Should the player be allowed to jump?<br>(0: No, 1: Yes) | 1
| 6 | Jump Strength | The default jumping strength off the rail, times 1000. | 25,000<br>(25.000)
| 7 | Allow Spinning | Should the player be allowed to spin?<br>(0: No, 1: Yes) | 1

## Switches
| Switch | Type | Effect
|---|---|---
|SW_A| Read | If set, only allows the player to grind if the switch is on.
|SW_B| Write | Turns SW_B on/off based on Obj_Arg 3

## ActionSound
This module uses custom ActionSound entries, you must add them to ActionSound using any bcsv editor.
```csv
GrindRail,JumpingOff,SE_PM_SKATE_JUMP,,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0,0,-1,-1,0,-1,-1
GrindRail,Attach,SE_PM_SKATE_LAND,,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0,0,-1,-1,0,-1,-1
```

## Effects
This module uses custom particles, you must import them yourself using pygapa.
```json
[
    {
        "GroupName": "GrindRail",
        "UniqueName": "Spark",
        "EffectName": [
            "BegomanSpark00",
            "BegomanSpark01"
        ],
        "OffsetX": 0.0,
        "OffsetY": 0.0,
        "OffsetZ": 0.0,
        "ScaleValue": 0.875,
        "RateValue": 0.75,
        "LightAffectValue": 0.0,
        "DrawOrder": "3D",
        "Affect": [
            "T"
        ]
    },
    {
        "GroupName": "GrindRail",
        "UniqueName": "Collision",
        "EffectName": [
            "WallHit00"
        ],
        "OffsetX": 0.0,
        "OffsetY": 0.0,
        "OffsetZ": 0.0,
        "ScaleValue": 3.0,
        "RateValue": 1.0,
        "LightAffectValue": 0.0,
        "DrawOrder": "3D",
        "Affect": [
            "T"
        ]
    }
]
```
