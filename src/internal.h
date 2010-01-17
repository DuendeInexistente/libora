
#ifndef _ORACOMMON_H
#define _ORACOMMON_H

#define ORA_CLEAR_ERROR(ora) { if ((((ora_document*) ora)->magic == _ORA_MAGIC_READ || ((ora_document*) ora)->magic == _ORA_MAGIC_WRITE)) ((ora_document *)ora)->error = 0; }
#define ORA_SET_ERROR(ora, id) ((ora_document *)ora)->error = id

#ifdef DEBUG
  #define ORA_DEBUG(...) { fprintf(stderr, "ORA(%s,%d): ",  __FUNCTION__, __LINE__); fprintf(stderr, __VA_ARGS__); }
//  #define _ORA_EXIT_ERROR(ora, id, retval) {ORA_SET_ERROR(ora, id); ORA_DEBUG("Error id: %d\n", id) return retval;}
#else
  #define ORA_DEBUG(...) {  }
#endif

typedef struct ora_document_d
{
    char magic;
    int flags;
    int error;
} ora_document;

#endif
