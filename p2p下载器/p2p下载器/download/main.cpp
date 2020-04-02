#define _CRT_SECURE_NO_WARNINGS
#include"client.h"
void helloworld(const httplib::Request &req, httplib::Response &rsp)
{
	printf("httplib server recv a req:%s\n", req.path.c_str());
	rsp.set_content("<html><h1>HelloWorld</h1></html>", "text/html");//响应添加正文
	rsp.status = 200;//响应状态码 200响应成功
}
void Scandir()
{
	const char *ptr = "./";
	boost::filesystem::directory_iterator begin(ptr);//定义一个目录迭代器对象
	boost::filesystem::directory_iterator end;
	for (; begin != end; ++begin){
		//begin->status() 目录中当前文件信息
		//boost::filesystem::is_directory()判断当前文件是否是一个目录
		if (boost::filesystem::is_directory(begin->status())){
			std::cout << begin->path().string() << "是一个目录\n";
		}
		else{
			std::cout << begin->path().string() << "是一个普通文件\n";
			//begin->path().filename()只要文件名不要路径
			std::cout << "文件名：" << begin->path().filename().string() <<std::endl;
		}
	}
}
void test(){
	//httplib::Server srv;
	//srv.Get("/", helloworld);
	//srv.listen("0.0.0.0", 9000);//本机任意Ip地址
	/*Scandir();
	Sleep(111111);*/
}
void ClientRun()
{
	Sleep(1);
	Client cli;
	cli.Start();
}
int main()
{
	//在主线程中要运行客户端模块以及服务端模块
	//创建线程运行客户端模块，主线程运行服务端模块
	std::thread thr_client(ClientRun);//若客户端运行时，服务端还没有启动
	
	//服务端模块
	Server srv;
	srv.Start();
	return 0;
}