
# Swarm 18ch DMX Mode

*ch1: Auto Program Select*
  - Used to enable each of 4 effects: Wash, Laser, Derby, Strobe
  - Maybe put this on it's own slider or buttons in TouchOSC. Could also toggle them with individual settings.

*ch2: Auto Program Speed*
  - 0-200:    Slow - Fast
  - 201-255:  Sound reactive, all effects

*ch3-10: Wash Group Color*
  - Each of the 8 wash groups can be either: Red, Green, Blue, or UV.
  - 1-51:     Off
  - 52-102:   Red
  - 103-153:  Green
  - 154-204:  Blue
  - 205-255:  UV
  - Or I may get lucky and 0-255 is a range representing a rainbow.

*ch11: Wash Strobe*
  - 0-255: Slow - Fast

*ch12: Derby Mode*
  - 0:        Off
  - 10-164:   Different values enable some combo of Red, Green, Blue, Amber, White
  - 223-255:  Sound Reactive Derby Mode

*ch13: Derby Strobe*
  - 0:      Off
  - 5-200:  Fast - Slow
  - 201-255 Sound Reactive Derby Strobe

*ch14: Derby Rotation*
  - 0-4:      Stop
  - 5-127:    Slow - Fast (CW)
  - 128-133:  Stop
  - 134-255:  Slow - Fast (CCW)

*ch15: Dedicated Strobe*
  - 0:        Blackout
  - 120-200:  Solid On
  - 201-255:  Sound Reactive Dedicated Strobe

*ch16: Lasers*
  - 0:    Blackout
  - 10:   Red Lasers
  - 50:   Green Lasers
  - 90:   Red/Green Strobe Alternating
  - 210:  Red/Green Strobe Together

*ch17: Laser Strobe*
  - 0:    Off
  - 255:  Sound Reactive Laser Strobe

*ch18: Motor LED*
  - 0-4:      Stop
  - 5-127:    Slow - Fast (CW)
  - 128-133:  Stop
  - 134-255:  Slow - Fast (CCW)


## Swarm FX Summary:
  - Channels I mostly care about: 1-10, 12, 14, 16, 18
