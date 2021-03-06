//===- jdwp.h - ART-DEBUGGER ------------------------------------*- C++ -*-===//
//
//                     ANDROID REVERSE TOOLKIT
//
// This file is distributed under the GNU GENERAL PUBLIC LICENSE
// V3 License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// jdwp.h contain some defination for jdwp msg
//
//===----------------------------------------------------------------------===//

#ifndef PROJECT_JDWP_H
#define PROJECT_JDWP_H

#include <QByteArray>
#include "utils/Defs.h"

namespace JDWP {

/*
 * Fundamental types.
 *
 * ObjectId and RefTypeId must be the same size.
 */
    typedef uint32_t FieldId;     /* static or instance field */
    typedef uint32_t MethodId;    /* any kind of method, including constructors */
    typedef uint64_t ObjectId;    /* any object (threadID, stringID, arrayID, etc) */
    typedef uint64_t RefTypeId;   /* like ObjectID, but unique for Class objects */
    typedef uint64_t FrameId;     /* short-lived stack frame ID */

    enum JdwpTag {
        JT_ARRAY                 = '[',
        JT_BYTE                  = 'B',
        JT_CHAR                  = 'C',
        JT_OBJECT                = 'L',
        JT_FLOAT                 = 'F',
        JT_DOUBLE                = 'D',
        JT_INT                   = 'I',
        JT_LONG                  = 'J',
        JT_SHORT                 = 'S',
        JT_VOID                  = 'V',
        JT_BOOLEAN               = 'Z',
        JT_STRING                = 's',
        JT_THREAD                = 't',
        JT_THREAD_GROUP          = 'g',
        JT_CLASS_LOADER          = 'l',
        JT_CLASS_OBJECT          = 'c',
    };

    enum JdwpTypeTag {
        TT_CLASS                = 1,
        TT_INTERFACE            = 2,
        TT_ARRAY                = 3,
    };

    enum JdwpEventKind {
        EK_SINGLE_STEP          = 1,
        EK_BREAKPOINT           = 2,
        EK_FRAME_POP            = 3,
        EK_EXCEPTION            = 4,
        EK_USER_DEFINED         = 5,
        EK_THREAD_START         = 6,
        EK_THREAD_DEATH         = 7,  // Formerly known as THREAD_END.
        EK_CLASS_PREPARE        = 8,
        EK_CLASS_UNLOAD         = 9,
        EK_CLASS_LOAD           = 10,
        EK_FIELD_ACCESS         = 20,
        EK_FIELD_MODIFICATION   = 21,
        EK_EXCEPTION_CATCH      = 30,
        EK_METHOD_ENTRY         = 40,
        EK_METHOD_EXIT          = 41,
        EK_METHOD_EXIT_WITH_RETURN_VALUE = 42,
        EK_MONITOR_CONTENDED_ENTER       = 43,
        EK_MONITOR_CONTENDED_ENTERED     = 44,
        EK_MONITOR_WAIT         = 45,
        EK_MONITOR_WAITED       = 46,
        EK_VM_START             = 90,  // Formerly known as VM_INIT.
        EK_VM_DEATH             = 99,
        EK_VM_DISCONNECTED      = 100,  // "Never sent across JDWP".
    };

    enum JdwpSuspendPolicy {
        SP_NONE                 = 0,    // Suspend no threads.
        SP_EVENT_THREAD         = 1,    // Suspend event thread.
        SP_ALL                  = 2,    // Suspend all threads.
    };

    enum JdwpModKind {
        MK_COUNT                = 1,
        MK_CONDITIONAL          = 2,
        MK_THREAD_ONLY          = 3,
        MK_CLASS_ONLY           = 4,
        MK_CLASS_MATCH          = 5,
        MK_CLASS_EXCLUDE        = 6,
        MK_LOCATION_ONLY        = 7,
        MK_EXCEPTION_ONLY       = 8,
        MK_FIELD_ONLY           = 9,
        MK_STEP                 = 10,
        MK_INSTANCE_ONLY        = 11,
        MK_SOURCE_NAME_MATCH    = 12,  // Since Java 6.
    };

    bool IsPrimitiveTag(JDWP::JdwpTag tag);

    size_t GetTagWidth(JDWP::JdwpTag tag);

    struct JValue {
        JdwpTag tag;
        union {
            // 0
            void* V;            // JT_VOID
            //1
            unsigned char B;    // JT_BYTE
            bool Z;             // JT_BOOLEAN
            //2
            int16_t C;          // JT_CHAR
            int16_t S;          // JT_SHORT
            //4
            float F;            // JT_FLOAT
            int32_t I;          // JT_INT
            //8
            ObjectId a;         // JT_ARRAY
            ObjectId L;         // JT_OBJECT
            ObjectId s;         // JT_STRING
            ObjectId t;         // JT_THREAD
            ObjectId g;         // JT_THREAD_GROUP
            ObjectId l;         // JT_CLASS_LOADER
            ObjectId c;         // JT_CLASS_OBJECT
            //8
            double D;           // JT_DOUBLE
            int64_t J;          // JT_LONG
        };
    public:
        JValue():tag(JT_VOID), V(nullptr) { }
        JValue(unsigned char B_):tag(JT_BYTE), B(B_) { }
        JValue(bool Z_):tag(JT_BOOLEAN), Z(Z_) { }
        JValue(int16_t C_, JDWP::JdwpTag tag_ = JT_CHAR):tag(tag_), C(C_) { }
        JValue(float F_):tag(JT_FLOAT), F(F_) { }
        JValue(int32_t I_):tag(JT_INT), I(I_) { }
        JValue(ObjectId L_, JDWP::JdwpTag tag_ = JT_OBJECT):tag(tag_), L(L_) { }
        JValue(double D_):tag(JT_DOUBLE), D(D_) { }
        JValue(int64_t J_):tag(JT_LONG), J(J_) { }

        JValue(JDWP::JdwpTag tag, const void* p) {
            this->tag = tag;
            auto w = GetTagWidth(tag);
            memcpy(V, p, w);
        }
        JValue(JDWP::JdwpTag tag) {
            this->tag = tag;
            V = nullptr;
        }
        bool operator == (const JValue& other) {
            if(this->tag != other.tag) {
                return false;
            }
            auto w = GetTagWidth(tag);
            auto s1 = this->L, s2 = other.L;
            while(w != 0) {
                if((s1 & 0xff) != (s2 & 0xff)) {
                    return false;
                }
                s1 >>= 4; s2 >>= 4;
                w--;
            }
            return true;
        }
    };

    struct ClassInfo {
        uint8_t    mRefTypeTag;	    // Kind of following reference type.
        RefTypeId  mTypeId;         // Loaded reference type
        QByteArray mDescriptor;    // The JNI signature of the loaded reference type.
        QByteArray mGenericSignature;  // The generic signature of the loaded reference type.
        uint32_t   mStatus;	        // The current class status.
    };

    struct MethodInfo {
        MethodId	 mMethodId;
        QByteArray	 mName;
        QByteArray	 mSignature;
        QByteArray	 mGenericSignature;
        uint32_t	 mFlags;
    };

    struct FieldInfo {
        FieldId	     mFieldId;
        QByteArray	 mName;
        QByteArray	 mDescriptor;
        QByteArray	 mGenericSignature;
        uint32_t	 mFlags;
        JValue       mValue;
    };

    struct JdwpLocation {
        JdwpTypeTag type_tag;
        RefTypeId class_id;
        MethodId method_id;
        uint64_t dex_pc;
        bool operator == (const JdwpLocation& loc);
    };

    union JdwpEventMod {
        JdwpModKind modKind;
        struct {
            JdwpModKind modKind;
            int         count;
        } count;
        struct {
            JdwpModKind modKind;
            uint32_t          exprId;
        } conditional;
        struct {
            JdwpModKind modKind;
            ObjectId    threadId;
        } threadOnly;
        struct {
            JdwpModKind modKind;
            RefTypeId   refTypeId;
        } classOnly;
        struct {
            JdwpModKind modKind;
            char*       classPattern;
        } classMatch;
        struct {
            JdwpModKind modKind;
            char*       classPattern;
        } classExclude;
        struct {
            JdwpModKind modKind;
            JdwpLocation loc;
        } locationOnly;
        struct {
            JdwpModKind modKind;
            uint8_t          caught;
            uint8_t          uncaught;
            RefTypeId   refTypeId;
        } exceptionOnly;
        struct {
            JdwpModKind modKind;
            RefTypeId   refTypeId;
            FieldId     fieldId;
        } fieldOnly;
        struct {
            JdwpModKind modKind;
            ObjectId    threadId;
            int         size;           /* JdwpStepSize */
            int         depth;          /* JdwpStepDepth */
        } step;
        struct {
            JdwpModKind modKind;
            ObjectId    objectId;
        } instanceOnly;
    };
    struct JdwpEventModPad {
        JdwpEventModPad(JdwpEventMod mod);
        ~JdwpEventModPad();
        JdwpEventMod mMod;
        QByteArray mArray;
    };
}
#endif //PROJECT_JDWP_H
