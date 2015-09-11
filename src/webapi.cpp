#include "webapi.h"

void doc::Response()
{
	string strExtName;
	string strDoc;
	string strPage;
	
	strDoc = m_session->GetQueryPage();
	if(m_session->m_cache->m_htdoc.find(strDoc) == m_session->m_cache->m_htdoc.end())
	{
		strDoc = m_work_path.c_str();
		strDoc += "/html/";
		strDoc += m_session->GetQueryPage();
		ifstream fsDoc(strDoc.c_str(), ios_base::binary);
		string strline;
		if(!fsDoc.is_open())
		{
            CHttpResponseHeader header;
            header.m_StatusCode = SC404;
            m_session->HttpSend(header.GetText(), strlen(header.GetText()));
		}
		else
		{
			lowercase(m_session->GetQueryPage(), strPage);				
			get_extend_name(strPage.c_str(), strExtName);
			
			CHttpResponseHeader header;
            header.m_StatusCode = SC200;
			
			if(m_session->m_cache->m_type_table.find(strExtName) == m_session->m_cache->m_type_table.end())
			{
                header.m_ContentType = "application/*";
			}
			else
			{
				header.m_ContentType = m_session->m_cache->m_type_table[strExtName];
			}
			m_session->HttpSend(header.GetText(), strlen(header.GetText()));
			
			char rbuf[1449];
			while(1)
			{
				if(fsDoc.eof())
				{
					break;
				}
				if(fsDoc.read(rbuf, 1448) < 0)
				{
					break;
				}
				int rlen = fsDoc.gcount();
				m_session->HttpSend(rbuf, rlen);
			}
		}
		if(fsDoc.is_open())
			fsDoc.close();
	}
	else
	{
		lowercase(m_session->GetQueryPage(), strPage);
		get_extend_name(strPage.c_str(), strExtName);
		
        CHttpResponseHeader header;
        header.m_StatusCode = SC200;
		
		if(m_session->m_cache->m_type_table.find(strExtName) == m_session->m_cache->m_type_table.end())
		{
			 header.m_ContentType = "application/*";
		}
		else
		{
			header.m_ContentType = m_session->m_cache->m_type_table[strExtName];
		}
		m_session->HttpSend(header.GetText(), strlen(header.GetText()));
		m_session->HttpSend(m_session->m_cache->m_htdoc[strDoc].pbuf, m_session->m_cache->m_htdoc[strDoc].flen);
		
	}
}
