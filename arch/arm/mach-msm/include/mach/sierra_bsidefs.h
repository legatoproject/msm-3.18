#ifndef BS_IDEFS_H
#define BS_IDEFS_H
/* Local constants and enumerated types */

/* RAM Copies of HW type, rev, etc. */
extern bool bshwconfigread;


/* Structures */

/************
 *
 * Name:    bshwconfig
 *
 * Purpose: to allow easy access to fields of the hardware configuration
 *
 * Members:
 *          all             - single uint32 containing all fields
 *          hw.family       - hardware family
 *          hw.type         - hardware type
 *          hw.rev          - hardware revision
 *          hw.spare        - spare
 *
 * Notes:
 *
 ************/
union bshwconfig
{
  uint32_t all;
  struct __packed
  {
    uint8_t family;             /* hardware family                     */
    uint8_t type;               /* hardware type                       */
    uint8_t rev;                /* hardware revision                   */
    uint8_t spare;              /* spare                               */
  } hw;
};

#endif
