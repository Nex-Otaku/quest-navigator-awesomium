#ifndef __METHOD_DISPATCHER_H__
#define __METHOD_DISPATCHER_H__

#include <Awesomium/WebCore.h>
#include "js_delegate.h"
#include <map>

//
// Common class that dispatches JS Method Calls to C++ Methods.
// Usage is something like:
//    MyClass my_class;
//    MethodDispatcher dispatcher;
//    my_web_view->set_js_method_handler(&dispatcher);
//    dispatcher.Bind(js_object,
//                    WSLit("JSMethodName"),
//                    JSDelegate(&my_class, &MyClass::CPPMethodName));
//
class MethodDispatcher : public Awesomium::JSMethodHandler {
 public:
  typedef std::pair<int, Awesomium::WebString> ObjectMethodKey;
  typedef std::map<ObjectMethodKey, JSDelegate> BoundMethodMap;
  typedef std::map<ObjectMethodKey, JSDelegateWithRetval> BoundMethodWithRetvalMap;

  MethodDispatcher();

  void Bind(Awesomium::JSObject& object,
    const Awesomium::WebString& name,
    JSDelegate callback);

  void BindWithRetval(Awesomium::JSObject& object,
    const Awesomium::WebString& name,
    JSDelegateWithRetval callback);

  // Inherited from JSMethodHandler
  void OnMethodCall(Awesomium::WebView* caller,
                    unsigned int remote_object_id, 
                    const Awesomium::WebString& method_name,
                    const Awesomium::JSArray& args);

  // Inherited from JSMethodHandler
  Awesomium::JSValue OnMethodCallWithReturnValue(Awesomium::WebView* caller,
                                      unsigned int remote_object_id,
                                      const Awesomium::WebString& method_name,
                                      const Awesomium::JSArray& args);

protected:
  BoundMethodMap bound_methods_;
  BoundMethodWithRetvalMap bound_methods_with_retval_;
};

#endif  // __METHOD_DISPATCHER_H__
