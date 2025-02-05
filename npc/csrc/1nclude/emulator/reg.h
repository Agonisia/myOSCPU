#ifndef __EMULATOR_REG_H__
#define __EMULATOR_REG_H__

#include "emulator/simulate.h"

void reg_display();
void reg_update();

word_t reg_str2val(const char *s, bool *success);

#endif //__EMULATOR_REG_H__
