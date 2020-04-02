#define _CRT_SECURE_NO_WARNINGS
#include"client.h"
void helloworld(const httplib::Request &req, httplib::Response &rsp)
{
	printf("httplib server recv a req:%s\n", req.path.c_str());
	rsp.set_content("<html><h1>HelloWorld</h1></html>", "text/html");//��Ӧ�������
	rsp.status = 200;//��Ӧ״̬�� 200��Ӧ�ɹ�
}
void Scandir()
{
	const char *ptr = "./";
	boost::filesystem::directory_iterator begin(ptr);//����һ��Ŀ¼����������
	boost::filesystem::directory_iterator end;
	for (; begin != end; ++begin){
		//begin->status() Ŀ¼�е�ǰ�ļ���Ϣ
		//boost::filesystem::is_directory()�жϵ�ǰ�ļ��Ƿ���һ��Ŀ¼
		if (boost::filesystem::is_directory(begin->status())){
			std::cout << begin->path().string() << "��һ��Ŀ¼\n";
		}
		else{
			std::cout << begin->path().string() << "��һ����ͨ�ļ�\n";
			//begin->path().filename()ֻҪ�ļ�����Ҫ·��
			std::cout << "�ļ�����" << begin->path().filename().string() <<std::endl;
		}
	}
}
void test(){
	//httplib::Server srv;
	//srv.Get("/", helloworld);
	//srv.listen("0.0.0.0", 9000);//��������Ip��ַ
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
	//�����߳���Ҫ���пͻ���ģ���Լ������ģ��
	//�����߳����пͻ���ģ�飬���߳����з����ģ��
	std::thread thr_client(ClientRun);//���ͻ�������ʱ������˻�û������
	
	//�����ģ��
	Server srv;
	srv.Start();
	return 0;
}