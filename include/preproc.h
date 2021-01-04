#ifndef PREPROCESS_H
#define PREPROCESS_H

#include "globaldef.h"

#ifdef __cplusplus
extern "C" {
#endif /* C++ */
// 0 is for realtime, 1 is only for wakeup time
void set_parameters(int doa_type);
int Preproc_mic_array_init();
int Preproc_mic_array_deinit();
int32_t Preproc_mic_array_read(void * buffer, int bytes, bool *triggerflag, bool *vadflag, int16_t *angle, int chnum);

#ifdef __cplusplus
} /* extern "C" */
#endif /* C++ */


#endif

