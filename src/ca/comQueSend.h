/*************************************************************************\
* Copyright (c) 2002 The University of Chicago, as Operator of Argonne
*     National Laboratory.
* Copyright (c) 2002 The Regents of the University of California, as
*     Operator of Los Alamos National Laboratory.
* EPICS BASE Versions 3.13.7
* and higher are distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution. 
\*************************************************************************/


/*  
 *  $Id$
 *
 *                              
 *                    L O S  A L A M O S
 *              Los Alamos National Laboratory
 *               Los Alamos, New Mexico 87545
 *                                  
 *  Copyright, 1986, The Regents of the University of California.
 *                                  
 *           
 *	Author Jeffrey O. Hill
 *	johill@lanl.gov
 *	505 665 1831
 */

#ifndef comQueSendh  
#define comQueSendh

#include <new> 

#include "tsDLList.h"
#include "comBuf.h"

//
// Notes.
// o calling popNextComBufToSend() will clear any uncommitted bytes
//
class comQueSend {
public:
    comQueSend ( wireSendAdapter &, comBufMemoryManager & ) epicsThrows (());
    ~comQueSend () epicsThrows (());
    void clear () epicsThrows (());
    void beginMsg () epicsThrows (());
    void commitMsg () epicsThrows (());
    unsigned occupiedBytes () const epicsThrows (());
    bool flushEarlyThreshold ( unsigned nBytesThisMsg ) const epicsThrows (());
    bool flushBlockThreshold ( unsigned nBytesThisMsg ) const epicsThrows (());
    bool dbr_type_ok ( unsigned type ) epicsThrows (());
    void pushUInt16 ( const ca_uint16_t value ) epicsThrows (());
    void pushUInt32 ( const ca_uint32_t value ) epicsThrows (());
    void pushFloat32 ( const ca_float32_t value ) epicsThrows (());
    void pushString ( const char *pVal, unsigned nChar ) epicsThrows (());
    void insertRequestHeader (
        ca_uint16_t request, ca_uint32_t payloadSize, 
        ca_uint16_t dataType, ca_uint32_t nElem, ca_uint32_t cid, 
        ca_uint32_t requestDependent, bool v49Ok ) 
            epicsThrows (( cacChannel::outOfBounds ));
    void insertRequestWithPayLoad (
        ca_uint16_t request, unsigned dataType, ca_uint32_t nElem, 
        ca_uint32_t cid, ca_uint32_t requestDependent, 
        const void * pPayload, bool v49Ok ) 
            epicsThrows (( cacChannel::outOfBounds, cacChannel::badType ));
    void push_dbr_type ( unsigned type, const void *pVal, unsigned nElem ) epicsThrows (());
    comBuf * popNextComBufToSend () epicsThrows (());
private:
    comBufMemoryManager & comBufMemMgr;
    tsDLList < comBuf > bufs;
    tsDLIter < comBuf > pFirstUncommited;
    wireSendAdapter & wire;
    unsigned nBytesPending;
    void copy_dbr_string ( const void *pValue, unsigned nElem ) epicsThrows (());
    void copy_dbr_short ( const void *pValue, unsigned nElem ) epicsThrows (());
    void copy_dbr_float ( const void *pValue, unsigned nElem ) epicsThrows (());
    void copy_dbr_char ( const void *pValue, unsigned nElem ) epicsThrows (());
    void copy_dbr_long ( const void *pValue, unsigned nElem ) epicsThrows (());
    void copy_dbr_double ( const void *pValue, unsigned nElem ) epicsThrows (());
    void pushComBuf ( comBuf & ) epicsThrows (());
    typedef void ( comQueSend::*copyFunc_t ) (  
        const void *pValue, unsigned nElem );
    static const copyFunc_t dbrCopyVector [39];

    void clearUncommitted () epicsThrows (());

    //
    // visual C++ versions 6 & 7 do not allow out of 
    // class member template function definition
    //
    template < class T >
    inline void push ( const T *pVal, const unsigned nElem ) epicsThrows (())
    {
        comBuf * pLastBuf = this->bufs.last ();
        unsigned nCopied;
        if ( pLastBuf ) {
            nCopied = pLastBuf->push ( pVal, nElem );
        }
        else {
            nCopied = 0u;
        }
        while ( nElem > nCopied ) {
            comBuf * pComBuf = new ( this->comBufMemMgr ) comBuf;
            unsigned nNew = pComBuf->push 
                        ( &pVal[nCopied], nElem - nCopied );
            nCopied += nNew;
            this->pushComBuf ( *pComBuf );
        }
    }

    //
    // visual C++ versions 6 and 7 do not allow out of 
    // class member template function definition
    //
    template < class T >
    inline void push ( const T & val ) epicsThrows (())
    {
        comBuf * pComBuf = this->bufs.last ();
        if ( pComBuf && pComBuf->push ( val ) ) {
            return;
        }
        pComBuf = new ( this->comBufMemMgr ) comBuf;
        assert ( pComBuf->push ( val ) );
        this->pushComBuf ( *pComBuf );
    }

    comQueSend ( const comQueSend & ) epicsThrows (());
    comQueSend & operator = ( const comQueSend & ) epicsThrows (());
};

extern const char cacNillBytes[];

inline bool comQueSend::dbr_type_ok ( unsigned type ) epicsThrows (())
{
    if ( type >= ( sizeof ( this->dbrCopyVector ) / sizeof ( this->dbrCopyVector[0] )  ) ) {
        return false;
    }
    if ( ! this->dbrCopyVector [type] ) {
        return false;
    }
    return true;
}

inline void comQueSend::pushUInt16 ( const ca_uint16_t value ) epicsThrows (())
{
    this->push ( value );
}

inline void comQueSend::pushUInt32 ( const ca_uint32_t value ) epicsThrows (())
{
    this->push ( value );
}

inline void comQueSend::pushFloat32 ( const ca_float32_t value ) epicsThrows (())
{
    this->push ( value );
}

inline void comQueSend::pushString ( const char *pVal, unsigned nChar ) epicsThrows (())
{
    this->push ( pVal, nChar );
}

// it is assumed that dbr_type_ok() was called prior to calling this routine
// to check the type code
inline void comQueSend::push_dbr_type ( unsigned type, const void *pVal, unsigned nElem ) epicsThrows (())
{
    ( this->*dbrCopyVector [type] ) ( pVal, nElem );
}

inline void comQueSend::pushComBuf ( comBuf & cb ) epicsThrows (())
{
    this->bufs.add ( cb );
    if ( ! this->pFirstUncommited.valid() ) {
        this->pFirstUncommited = this->bufs.lastIter ();
    }
}

inline unsigned comQueSend::occupiedBytes () const epicsThrows (())
{
    return this->nBytesPending;
}

inline bool comQueSend::flushBlockThreshold ( unsigned nBytesThisMsg ) const epicsThrows (())
{
    return ( this->nBytesPending + nBytesThisMsg > 16 * comBuf::capacityBytes () );
}

inline bool comQueSend::flushEarlyThreshold ( unsigned nBytesThisMsg ) const epicsThrows (())
{
    return ( this->nBytesPending + nBytesThisMsg > 4 * comBuf::capacityBytes () );
}

inline void comQueSend::beginMsg () epicsThrows (())
{
    if ( this->pFirstUncommited.valid() ) {
        this->clearUncommitted ();
    }
    this->pFirstUncommited = this->bufs.lastIter ();
}

#endif // ifndef comQueSendh
