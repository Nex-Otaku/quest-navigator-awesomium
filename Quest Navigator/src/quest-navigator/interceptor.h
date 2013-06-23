#ifndef INTERCEPTOR_H_
#define INTERCEPTOR_H_

#include <Awesomium/WebCore.h>
#include "../common/application.h"

using namespace Awesomium;

class QnInterceptor : public ResourceInterceptor
{
private:
	Application* app_;
public:
	virtual Awesomium::ResourceResponse* OnRequest(ResourceRequest* request);
	virtual bool OnFilterNavigation(int origin_process_id,
		int origin_routing_id,
		const WebString& method,
		const WebURL& url,
		bool is_main_frame);
	virtual void OnWillDownload(int origin_process_id,
		int origin_routing_id,
		const WebURL& url);

	QnInterceptor();
	void setApp(Application* app);
};

#endif
