#ifndef __ApplicationConfiguration_h
#define __ApplicationConfiguration_h

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
  mpiNone,
  mpiSerial,
  mpiUSB
} MCU_to_Piano_Interface_t;

// NB: USB with Yamaha Clavinova has been unreliable:
// => Pedal notes stop working after a random number of pedal notes pressed.
// => Any currently held keyboard notes stop playing the instant the pedal notes stop working.
// => Subsequently pressed keyboard notes work OK.
MCU_to_Piano_Interface_t MCU_to_Piano_Interface = mpiSerial;

#ifdef __cplusplus
}
#endif

#endif
