//===- DebugHandler.cpp - ART-DebugHandler --------------------------*- C++ -*-===//
//
//                     ANDROID REVERSE TOOLKIT
//
// This file is distributed under the GNU GENERAL PUBLIC LICENSE
// V3 License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
#include "DebugHandler.h"

#include "DebugSocket.h"

#include <QDebug>
#include <utils/CmdMsgUtil.h>
#include <Jdwp/jdwp.h>
#include <QTimer>
#include <Jdwp/JdwpHeader.h>

DebugHandler::DebugHandler(QObject *parent, DebugSocket* socket)
        : QObject(parent)
{
    mSocket = socket;

    connect(mSocket, &DebugSocket::newJDWPRequest, this, &DebugHandler::onJDWPRequest);

    connect(mSocket, &DebugSocket::error, this, &DebugHandler::onSocketError);
    connect(mSocket, &DebugSocket::connected, this, &DebugHandler::onSocketConnected);
    connect(mSocket, &DebugSocket::disconnected, this, &DebugHandler::onSocketDisconnected);

    connect(this, SIGNAL(sendBuffer(const char*,quint64)),
            mSocket->mSocketEvent, SLOT(onWrite(const char*,quint64)));
    connect(this, SIGNAL(sendBuffer(QByteArray)), mSocket->mSocketEvent, SLOT(onWrite(QByteArray)));

    connect(this, &DebugHandler::stopCurrentTarget, mSocket->mSocketEvent, &DebugSocketEvent::onStop);
}

DebugHandler::~DebugHandler()
{

}


void DebugHandler::onJDWPRequest (QByteArray data)
{
    JDWP::Request request((uint8_t*)data.data(), data.length());
    if(request.isReply ()) {
        handleReply (request);
    } else {
        handleCommand(request);
    }
}

void DebugHandler::handleReply (JDWP::Request &reply)
{
    auto id = reply.GetId ();

    auto it = mRequestMap.find (id);
    if(it == mRequestMap.end ()) {
        return;
    }
    auto request = it->data ();
    auto array = reply.GetExtraArray();
    request->handleReply(array);
    mRequestMap.erase (it);
}

void DebugHandler::handleCommand (JDWP::Request &request)
{
    // Command is sended form VM to debugger,
    qDebug() << "Handle command for id " << request.GetId()
             << " CommandSet:" << request.GetCommandSet ()
             << " Command:" << request.GetCommand ();
    JDWP::Composite::ReflectedType composite(request.GetExtra(), request.GetExtraLen());
    qDebug() << "Composite suspend" << composite.mSuspendPolicy << "even count" << composite.mCount;
    for(auto &pevent: composite.mEventList) {
        auto eventKind = pevent->mEventKind;
        switch(eventKind) {
            case JDWP::JdwpEventKind::EK_SINGLE_STEP:
            case JDWP::JdwpEventKind::EK_BREAKPOINT:
            case JDWP::JdwpEventKind::EK_METHOD_ENTRY:
            case JDWP::JdwpEventKind::EK_METHOD_EXIT: {
                auto event = (JDWP::Composite::ReflectedType::EventLocationEvent*)pevent;
                qDebug() << "EventLocatoinEvent" << event->mLocation.dex_pc;
            }
                break;
//            case JdwpEventKind::EK_FRAME_POP:
//                Q_ASSERT(false);
//                break;
            case JDWP::JdwpEventKind::EK_EXCEPTION: {
                auto event = (JDWP::Composite::ReflectedType::EventException*)pevent;
                qDebug() << "EventException at" << event->mThrowLoc.dex_pc << "to" << event->mCatchLoc.dex_pc;
            }
                break;
//            case JdwpEventKind::EK_USER_DEFINED:
//                Q_ASSERT(false);
//                break;
            case JDWP::JdwpEventKind::EK_THREAD_START:
            case JDWP::JdwpEventKind::EK_THREAD_DEATH: {
                auto event = (JDWP::Composite::ReflectedType::EventThreadChange*)pevent;
                qDebug() << "EventThreadChange" << event->mThreadId;
            }
                break;
            case JDWP::JdwpEventKind::EK_CLASS_PREPARE: {
                auto event = (JDWP::Composite::ReflectedType::EventClassPrepare*)pevent;
                qDebug() << "EventClassPrepare" << event->mSignature << event->mStatus;
            }
                break;
//            case JdwpEventKind::EK_CLASS_UNLOAD:
//                Q_ASSERT(false);
//                break;
//            case JdwpEventKind::EK_CLASS_LOAD:
//                Q_ASSERT(false);
//                break;
//            case JdwpEventKind::EK_FIELD_ACCESS:
//                Q_ASSERT(false);
//                break;
//            case JdwpEventKind::EK_FIELD_MODIFICATION:
//                Q_ASSERT(false);
//                break;
//            case JdwpEventKind::EK_EXCEPTION_CATCH:
//                Q_ASSERT(false);
//                break;
//            case JdwpEventKind::EK_METHOD_EXIT_WITH_RETURN_VALUE:
//                Q_ASSERT(false);
//                break;
//            case JdwpEventKind::EK_MONITOR_CONTENDED_ENTER:
//                Q_ASSERT(false);
//                break;
//            case JdwpEventKind::EK_MONITOR_CONTENDED_ENTERED:
//                Q_ASSERT(false);
//                break;
//            case JdwpEventKind::EK_MONITOR_WAIT:
//                Q_ASSERT(false);
//                break;
//            case JdwpEventKind::EK_MONITOR_WAITED:
//                Q_ASSERT(false);
//                break;
            case JDWP::JdwpEventKind::EK_VM_START: {
                auto event = (JDWP::Composite::ReflectedType::EventVmStart*)pevent;
                qDebug() << "EventVmStart" << event->mThreadId;
            }
                break;
            case JDWP::JdwpEventKind::EK_VM_DEATH: {
                //auto event = (JDWP::Composite::ReflectedType::EventVmDeath*)pevent;
                qDebug() << "EventVmDeath";
            }
                break;
//            case JdwpEventKind::EK_VM_DISCONNECTED:
//                Q_ASSERT(false);
//                break;
            default:
                qDebug() << "Unknown EventKind?" << eventKind;
                break;
        }
    }
}

bool DebugHandler::sendNewRequest (QSharedPointer<ReqestPackage>& req)
{
    mRequestMap[req->mRequest.GetId ()] = req;
    sendBuffer (req->mData);
    return true;
}

void DebugHandler::onSocketError(int error, const QString &message)
{
    qDebug() << "Socket error code : " << error << " msg: " << message;
}

void DebugHandler::onSocketConnected()
{
    cmdmsg()->addCmdMsg("DebugHandler connect to " + mSocket->hostName () + ":" +
                        QString::number(mSocket->port ()));
    mSockId = 1;
    mRequestMap.clear ();

    // Init
    dbgEventRequestSet(JDWP::JdwpEventKind::EK_CLASS_PREPARE, JDWP::JdwpSuspendPolicy::SP_NONE);
    dbgEventRequestSet(JDWP::JdwpEventKind::EK_CLASS_UNLOAD, JDWP::JdwpSuspendPolicy::SP_NONE);
    // suspend and set breakpoiont to catch exception
    {
        std::vector<JDWP::JdwpEventMod> mod;

        JDWP::JdwpEventMod m1;
        m1.modKind = JDWP::JdwpModKind::MK_CLASS_MATCH;
        std::string m1Class = "java.lang.Throwable";
        m1.classMatch.classPattern = (char*)m1Class.c_str();
        mod.push_back(m1);

        JDWP::JdwpEventMod m2;
        m2.modKind = JDWP::JdwpModKind::MK_COUNT;
        m2.count.count = 1;
        mod.push_back(m2);

        dbgEventRequestSet(JDWP::JdwpEventKind::EK_CLASS_PREPARE, JDWP::JdwpSuspendPolicy::SP_ALL, mod);
    }
    // suspend and set breakpoint to process entry point
    {
        std::vector<JDWP::JdwpEventMod> mod;

        JDWP::JdwpEventMod m1;
        m1.modKind = JDWP::JdwpModKind::MK_CLASS_MATCH;
        std::string m1Class = "com.example.ring.myapplication.MainActivity";
        m1.classMatch.classPattern = (char*)m1Class.c_str();
        mod.push_back(m1);

        JDWP::JdwpEventMod m2;
        m2.modKind = JDWP::JdwpModKind::MK_COUNT;
        m2.count.count = 1;
        mod.push_back(m2);

        dbgEventRequestSet(JDWP::JdwpEventKind::EK_CLASS_PREPARE, JDWP::JdwpSuspendPolicy::SP_ALL, mod);
    }

    QTimer::singleShot(10000, [this]() {
        // wait for classloading finished
        dbgVersion();
//        dbgAllClassesWithGeneric();
        // TODO set breakpoint and resume
        dbgSetBreakPoint("Lcom/example/ring/myapplication/MainActivity;", "onCreate", "(Landroid/os/Bundle;)V", 0);
//        dbgResume();
    });
}

void DebugHandler::onSocketDisconnected()
{
    cmdmsg()->addCmdMsg("DebugHandler disconnected");
    m_event.exit();
    mSockId = 1;
    mRequestMap.clear ();
}

// -------------------for debug interface----------------------
void DebugHandler::dbgVersion() {
    auto request = JDWP::VirtualMachine::Version::buildReq (mSockId++);
    auto package = QSharedPointer<ReqestPackage>(new ReqestPackage(request));
    connect(package.data(), &ReqestPackage::onReply, [this](JDWP::Request *request,QByteArray& reply) {
        JDWP::VirtualMachine::Version ver((uint8_t*)reply.data(), reply.length());
        qDebug() << "VirtualMachine::Version";
        qDebug() << ver.version << "major" << ver.major << "monor" << ver.minor
                 << "javaVersion" << ver.javaVersion << "javaVmName" << ver.javaVmName;
    });
    sendNewRequest (package);
}

void DebugHandler::dbgAllClassesWithGeneric() {
    auto request = JDWP::VirtualMachine::AllClassesWithGeneric::buildReq (mSockId++);
    auto package = QSharedPointer<ReqestPackage>(new ReqestPackage(request));
    connect(package.data(), &ReqestPackage::onReply, [this](JDWP::Request *request,QByteArray& reply) {
        qDebug() << "VirtualMachine::AllClassesWithGeneric: ";
        JDWP::VirtualMachine::AllClassesWithGeneric signature((uint8_t*)reply.data(), reply.length());
        for(auto& info: signature.mInfos) {
            qDebug() << "RefTypeTag" << info.mRefTypeTag
                     << "RefTypeId" << info.mTypeId
                     << "Descriptor" << info.mDescriptor
                     << "GenericSignature" << info.mGenericSignature
                     << "Status" << info.mStatus;
        }
        // TODO Record ClassInfo;
    });
    sendNewRequest (package);
}

void DebugHandler::dbgResume()
{
    auto request = JDWP::VirtualMachine::Resume::buildReq (mSockId++);
    auto package = QSharedPointer<ReqestPackage>(new ReqestPackage(request));
    connect(package.data(), &ReqestPackage::onReply, [this](JDWP::Request *request,QByteArray& reply) {
        dbgOnResume();
    });
    sendNewRequest (package);
}

void DebugHandler::dbgSetBreakPoint(const QString &classSignature,
                                    const QString &methodName,
                                    const QString &methodSign, uint64_t codeIdx)
{
    QSharedPointer<RequestExtraBreakPoint> extra(new RequestExtraBreakPoint(
            classSignature, methodName, methodSign, codeIdx));
    dbgGetClassBySignature(classSignature, [this, extra](JDWP::ClassInfo classinfo) {
        dbgGetRefTypeMethodsWithGeneric(classinfo.mTypeId, [this, extra, classinfo](QVector<JDWP::MethodInfo> methods) {
            for(auto &method: methods) {
                if(method.mName != extra->mMethodName || method.mSignature != extra->mMethodSign) {
                    continue;
                }
                qDebug() << "set BreakPoint to " << extra->mClassSignature << extra->mMethodName
                         << extra->mMethodSign << extra->mCodeIdx;
                std::vector<JDWP::JdwpEventMod> mod;
                JDWP::JdwpEventMod bpMod;
                bpMod.modKind = JDWP::MK_LOCATION_ONLY;
                bpMod.locationOnly.loc = JDWP::JdwpLocation{
                        JDWP::TT_CLASS, classinfo.mTypeId, method.mMethodId, extra->mCodeIdx};
                mod.push_back(bpMod);
                dbgEventRequestSet(JDWP::EK_BREAKPOINT, JDWP::SP_ALL, mod);
                dbgResume();
                break;
            }
        });
    });
}

void DebugHandler::dbgEventRequestSet(JDWP::JdwpEventKind eventkind,
                                      JDWP::JdwpSuspendPolicy policy,
                                      const std::vector<JDWP::JdwpEventMod> &mod) {
    auto request = JDWP::EventRequest::Set::buildReq(eventkind, policy, mod, mSockId++);
    auto package = QSharedPointer<ReqestPackage>(new ReqestPackage(request));
    connect(package.data(), &ReqestPackage::onReply, [this](JDWP::Request *request,QByteArray& reply) {
        JDWP::EventRequest::Set set((uint8_t*)reply.data(), reply.length());
        qDebug() << "EventRequestSet: " << set.mRequestId;
        // TODO Record event id
    });
    sendNewRequest (package);
}

template  <typename Func>
bool DebugHandler::dbgGetRefTypeMethodsWithGeneric(JDWP::RefTypeId refTypeId, Func callback) {
    auto request = JDWP::ReferenceType::MethodsWithGeneric::buildReq(refTypeId, mSockId++);
    auto package = QSharedPointer<ReqestPackage>(new ReqestPackage(request));
    connect(package.data(), &ReqestPackage::onReply, [this, callback](JDWP::Request *request,QByteArray& reply) {
            qDebug() << "ReferenceType::MethodsWithGeneric: ";
        JDWP::ReferenceType::MethodsWithGeneric signature((uint8_t*)reply.data(), reply.length());;
        if(signature.mSize == 0) {
            return;
        }
        for(auto& info: signature.mMethods) {
            qDebug() << "MethodId" << info.mMethodId
                     << "Name" << info.mName
                     << "Signature" << info.mSignature
                     << "GenericSignature" << info.mGenericSignature
                     << "Flags" << info.mFlags;
        }
        // TODO Record classInfo
        callback(signature.mMethods);
    });
    sendNewRequest (package);
    return true;
}

template <typename Func>
bool DebugHandler::dbgGetClassBySignature(const QString &classSignature, Func callback) {
    auto request = JDWP::VirtualMachine::ClassesBySignature::buildReq(classSignature.toLocal8Bit(), mSockId++);
    auto package = QSharedPointer<ReqestPackage>(new ReqestPackage(request));
    connect(package.data(), &ReqestPackage::onReply, [this, callback](JDWP::Request *request,QByteArray& reply) {
        JDWP::VirtualMachine::ClassesBySignature signature((uint8_t*)reply.data(), reply.length());
        // TODO classes has been loaded by multiply classloader, how can I get the info?
        if(signature.mSize == 0) {
            return;
        }
        qDebug() << "VirtualMachine::ClassBySignature: ";
        for(auto& info: signature.mInfos) {
            qDebug() << info.mRefTypeTag << info.mTypeId << info.mStatus;
        }
        auto info = signature.mInfos.front();
        if(info.mTypeId == 0) {
            return;
        }
        // TODO Record classInfo
        callback(info);
    });
    sendNewRequest (package);
    return true;
}

ReqestPackage::ReqestPackage(QByteArray& data,  QObject *parent)
    : QObject(parent),
      mData(data),
      mRequest((uint8_t*)data.data (), data.length())
{

}

ReqestPackage::~ReqestPackage()
{

}

void ReqestPackage::handleReply(QByteArray& reply)
{
    qDebug() << "Handle reply for id " << mRequest.GetId()
             << " CommandSet:" << mRequest.GetCommandSet ()
             << " Command:" << mRequest.GetCommand ();
    onReply(&mRequest, reply);
}
