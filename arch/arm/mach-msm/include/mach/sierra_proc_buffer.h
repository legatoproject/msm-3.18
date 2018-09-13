/* arch/arm/mach-msm/sierra_proc_buffer.h
 */

#ifndef SIERRA_PROC_BUFFER_H
#define SIERRA_PROC_BUFFER_H

/* NOTE: this file is also used by LK so please keep this file generic */

#define SIERRA_PROC_STRING_MAX_LEN 127

int sierra_get_dual_system_proc_buffer(char * proc_buffer, int len);

#endif /* SIERRA_PROC_BUFFER_H */
