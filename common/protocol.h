#ifndef  PROTOCOL_H_
#define  PROTOCOL_H_



typedef unsigned char protocol_cmd_type;

// define constants for my custom protocol command types
#define PROTOCOL_CMD_EXIT                       ((protocol_cmd_type) 0)
#define PROTOCOL_CMD_READ_FROM_HISTORY          ((protocol_cmd_type) 1)
#define PROTOCOL_CMD_EXECUTE_SHELL_CMD          ((protocol_cmd_type) 2)


// define constants for my custom protocol response end
#define PROTOCOL_RES_END               ((char) 0)



#endif   // PROTOCOL_H_
