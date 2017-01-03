//===- ProjectInfo.cpp - ART-GUI utilpart -----------------------*- C++ -*-===//
//
//                     ANDROID REVERSE TOOLKIT
//
// This file is distributed under the GNU GENERAL PUBLIC LICENSE
// V3 License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
#include "utils/ProjectInfo.h"
#include "utils/StringUtil.h"
#include <utils/Configuration.h>
#include <utils/ProjectInfo.h>



//ProjectInfo *ProjectInfo::instance ()
//{
//    static ProjectInfo* mInstance = nullptr;
//    if(mInstance == nullptr) {
//        mInstance = new ProjectInfo;
//    }
//    return mInstance;
//}
//
//QString ProjectInfo::getInfo(QString key)
//{
//    return mMap[key];
//}
//
//void ProjectInfo::setInfo(QString key, QString value)
//{
//    mMap[key] = value;
//}
//
//void ProjectInfo::reset()
//{
//    mMap.clear();
//}
//
//ProjectInfo::ProjectInfo()
//{
//
//}
//
//QString ProjectInfo::getProjectPath ()
//{
//    return GetProjectsPath (mMap["ProjectName"]);
//}
//
//QString ProjectInfo::getProjectCfgLastPath ()
//{
//    return GetProjectsPath (mMap["ProjectLast"]) + "/Config.xml";
//}
//
//QString ProjectInfo::getProjectCfgCurPath ()
//{
//    return GetProjectsPath (mMap["ProjectCur"]) + "/Config.xml";
//}
//
//bool ProjectInfo::isProjectOpened ()
//{
//    return !mMap["ProjectName"].isEmpty ();
//}

ProjectInfo::SConfig ProjectInfo::m_sconfig;

ProjectInfo* ProjectInfo::m_current = nullptr;
ProjectInfo* ProjectInfo::m_last = nullptr;
QMap<QString, ProjectInfo*> ProjectInfo::m_infoMap;

ProjectInfo *ProjectInfo::openProject(QString name) {
    if(m_infoMap.contains(name)) {
        return m_infoMap[name];
    }
    Q_ASSERT(m_current == nullptr && "current project not closed???");

    m_current = new ProjectInfo(name);
    m_infoMap[name] = m_current;
    return m_current;
}

bool ProjectInfo::closeProject() {
    if(m_current == nullptr) {
        return false;
    }

    m_last = m_current;
    m_current = nullptr;
    return true;
}

ProjectInfo::ProjectInfo(QString name) {
    // TODO load conformation from configuration
    m_config.m_projectName = name;
    Configuration cfg(getConfigPath());
    m_config.m_compileCmd = cfg.getString("ProjectInfo", "CompileCmd");
    m_config.m_decompileCmd = cfg.getString("ProjectInfo", "DecompileCmd");
}

QString ProjectInfo::getRootPath() {
    return GetProjectsPath(m_config.m_projectName);
}

QString ProjectInfo::getSourcePath() {
    return getRootPath() + "/Project";
}

QString ProjectInfo::getBuildPath() {
    return getRootPath() + "/Bin";
}

QString ProjectInfo::getConfigPath() {
    return getRootPath() + "/Config.xml";
}

void ProjectInfo::saveAllConfig() {
    for(auto &info: m_infoMap) {
        Configuration cfg(info->getConfigPath());
        cfg.setString ("ProjectInfo", "CompileCmd", info->m_config.m_compileCmd);
        cfg.setString ("ProjectInfo", "DecompileCmd", info->m_config.m_decompileCmd);
    }

}




