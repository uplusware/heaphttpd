/*
	Copyright (c) openheap, uplusware
	uplusware@gmail.com
*/

#include "httpbalancer.h"
#include "serviceobj.h"
#include "tinyxml/tinyxml.h"

typedef struct
{
    string host;
    unsigned char weight;    
} http_balance_server;

class http_server_group : public IServiceObj
{
public:
    http_server_group()
    {
        TiXmlDocument xmlFileterDoc;
        xmlFileterDoc.LoadFile("/etc/heaphttpd/extension/httpbalancer.xml");
        TiXmlElement * pRootElement = xmlFileterDoc.RootElement();
        if(pRootElement)
        {
            TiXmlNode* pChildNode = pRootElement->FirstChild("redirect");
            while(pChildNode)
            {
                if(pChildNode && pChildNode->ToElement())
                {
                    http_balance_server bal_svr;
                    bal_svr.host = pChildNode->ToElement()->Attribute("host") ? pChildNode->ToElement()->Attribute("host") : "";
                    if(bal_svr.host != "")
                    {
                        bal_svr.weight = (pChildNode->ToElement()->Attribute("weight") && pChildNode->ToElement()->Attribute("weight")[0] != '\0') ? atoi(pChildNode->ToElement()->Attribute("weight")) : 8;
                        m_http_servers.push_back(bal_svr);
                    }
                }
                pChildNode = pChildNode->NextSibling("redirect");
            }
        }
    }
    vector<http_balance_server> m_http_servers;
    
protected:
    virtual ~http_server_group()
    {
        
    }
};

void* ext_request(CHttp* http_session, const char * name, const char * description, const char * parameters, BOOL * skip)
{
    string strObjName = "extension_";
    strObjName += name;
	http_server_group* svr_grp = (http_server_group*)http_session->GetServiceObject(strObjName.c_str());
    
	if(svr_grp == NULL)
	{
		svr_grp = new http_server_group();
		http_session->SetServiceObject(strObjName.c_str(), svr_grp);
	}
    
    if(svr_grp->m_http_servers.size() > 0)
    {
        int index = http_session->m_clientip == "" ? 0 : http_session->m_clientip.c_str()[http_session->m_clientip.length() - 1];
        index = index % svr_grp->m_http_servers.size();
        
        string strLocation = svr_grp->m_http_servers[index].host.c_str();
        if(http_session->GetResource()[0] != '/')
            strLocation += "/";
        strLocation += http_session->GetResource();
        CHttpResponseHdr header;
        header.SetStatusCode(SC301);
        header.SetField("Location", strLocation.c_str());
        header.SetField("Content-Type", "text/html");
		header.SetField("Content-Length", header.GetDefaultHTMLLength());
        
        http_session->SendHeader(header.Text(), header.Length());
		http_session->SendContent(header.GetDefaultHTML(), header.GetDefaultHTMLLength());
        
        *skip = TRUE;
    }
}
