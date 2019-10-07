/*******************************************************************************
 * Project:  Nebula
 * @file     CmdOnNodeNotice.hpp
 * @brief    来自注册中心的节点通知
 * @author   Bwar
 * @date:    2019年9月14日
 * @note
 * Modify history:
 ******************************************************************************/

#include "CmdOnNodeNotice.hpp"

#include "ios/Dispatcher.hpp"
#include "actor/session/sys_session/manager/SessionManager.hpp"

namespace neb
{

CmdOnNodeNotice::CmdOnNodeNotice(int32 iCmd)
    : Cmd(iCmd), m_pSessionManager(nullptr)
{
}

CmdOnNodeNotice::~CmdOnNodeNotice()
{
}

bool CmdOnNodeNotice::AnyMessage(
        std::shared_ptr<SocketChannel> pChannel,
        const MsgHead& oInMsgHead,
        const MsgBody& oInMsgBody)
{
    CJsonObject oJson;
    if (!oJson.Parse(oInMsgBody.data()))
    {
        LOG4_ERROR("failed to parse msgbody content!");
        return(false);
    }
    if (m_pSessionManager == nullptr)
    {
        m_pSessionManager = std::dynamic_pointer_cast<SessionManager>(
                GetSession("neb::SessionManager"));
        if (m_pSessionManager == nullptr)
        {
            LOG4_ERROR("no session named \"neb::SessionManager\"!");
            return(false);
        }
    }
    m_pSessionManager->SendToWorker(CMD_REQ_NODE_NOTICE, GetSequence(), oInMsgBody);
    std::ostringstream oss;
    std::string strIdentify;
    std::string strNodeType;
    std::string strNodeHost;
    int iNodePort = 0;
    int iWorkerNum = 0;
    for (int i = 0; i < oJson["add_nodes"].GetArraySize(); ++i)
    {
        if (oJson["add_nodes"][i].Get("node_type",strNodeType)
                && oJson["add_nodes"][i].Get("node_ip",strNodeHost)
                && oJson["add_nodes"][i].Get("node_port",iNodePort)
                && oJson["add_nodes"][i].Get("worker_num",iWorkerNum))
        {
            oss << strNodeHost << ":" << iNodePort;
            strIdentify = std::move(oss.str());
            m_pSessionManager->AddOnlineNode(strIdentify, oJson["add_nodes"][i].ToString());
            if (std::string("LOGGER") == strNodeType)
            {
                for(int j = 1; j <= iWorkerNum; ++j)
                {
                    oss << strNodeHost << ":" << iNodePort << "." << j;
                    strIdentify = std::move(oss.str());
                    GetLabor(this)->GetDispatcher()->AddNodeIdentify(strNodeType, strIdentify);
                    LOG4_DEBUG("AddNodeIdentify(%s,%s)", strNodeType.c_str(), strIdentify.c_str());
                }
            }
        }
    }

    for (int i = 0; i < oJson["del_nodes"].GetArraySize(); ++i)
    {
        if (oJson["del_nodes"][i].Get("node_type",strNodeType)
                && oJson["del_nodes"][i].Get("node_ip",strNodeHost)
                && oJson["del_nodes"][i].Get("node_port",iNodePort)
                && oJson["del_nodes"][i].Get("worker_num",iWorkerNum))
        {
            oss << strNodeHost << ":" << iNodePort;
            strIdentify = std::move(oss.str());
            m_pSessionManager->DelOnlineNode(strIdentify);
            if (std::string("LOGGER") == strNodeType)
            {
                for(int j = 1; j <= iWorkerNum; ++j)
                {
                    oss << strNodeHost << ":" << iNodePort << "." << j;
                    strIdentify = std::move(oss.str());
                    if (std::string("LOGGER") == strNodeType)
                    {
                        GetLabor(this)->GetDispatcher()->DelNodeIdentify(strNodeType, strIdentify);
                        LOG4_DEBUG("DelNodeIdentify(%s,%s)", strNodeType.c_str(), strIdentify.c_str());
                    }
                }
            }
        }
    }
    return(true);
}

} /* namespace neb */