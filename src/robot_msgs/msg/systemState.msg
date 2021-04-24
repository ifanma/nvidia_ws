int8 STATE_DISABLE = -1
int8 STATE_IDLE = 0
int8 STATE_ONMOVEL = 1
int8 STATE_ONMOVEJ = 2
int8 STATE_ONMOVEFOLLOW = 3
int8 STATE_ONCARMOVE = 4
int8 STATE_ONMOVETEST = 5

int8 MOTORSTATE_DISABLE = 0
int8 MOTORSTATE_ENABLE = 1
int8 MOTORSTATE_ONENABLE = 2

int8 FORCECTRL_WATINGDATA = -2
int8 FORCECTRL_SLOWCLOSE = -1
int8 FORCECTRL_DISABLE = 0
int8 FORCECTRL_ENABLE = 1
int8 FORCECTRL_RAPIDCLOSE = 2

Header header
float32 voltage
float32 time_elapsed

int8[] left_motor_state
int8 leftarm_state
int8 leftarm_fctrl_state

int8[] right_motor_state
int8 rightarm_state
int8 rightarm_fctrl_state

int8[] head_motor_state
int8 head_state

int8[] leg_motor_state
int8 leg_state

int8[] trc_motor_state
int8 trc_state