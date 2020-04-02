#pragma once
#include<thread>  //�̵߳�ͷ�ļ�
#include"util.h"
#include"httplib.h"
#include <boost/filesystem.hpp>
#define P2P_PORT 9000 //900ǧ�˿�
#define MAX_IPBUFFER 16  //ת�����ip��ַ���뻺����,�������Ĵ�С
#define SHARED_PATH "./Shared" //����Ŀ¼
#define DOWNLOAD_PATH "./download"
class Host
{
public:
	int _ip_addr; //Ҫ��Ե�����IP��ַ
	bool _pair_ret;//���ڴ����Խ������Գɹ� true
};
class Client
{
public:
	bool Start()
	{
		//�ͻ��˳�����Ҫѭ�����У���Ϊ�����ļ�����ֻ����һ��
		//ѭ������ÿ������һ���ļ�֮�󶼻����������Բ�������
		while (1)
		{
			GetonlineHost();
		}
		return true;
	}
	//��������߳���ں���
	void HostPair(Host *host)
	{
		//1.��֯HTTPЭ���ʽ����������
		//2.�һ��TCP�ͻ��ˣ������ݷ���
		//3.�ȴ��������˵Ļظ��������н���

		//�������ʹ�õ�������httplibʵ�֣�ע���˴���֪httplib��������ʵ������
		host->_pair_ret = false;
		char buf[MAX_IPBUFFER] = { 0 };
		inet_ntop(AF_INET, &host->_ip_addr, buf, MAX_IPBUFFER);
		httplib::Client cli(buf, P2P_PORT); //ʵ����httplib�ͻ��˶���
		auto rsp = cli.Get("/hostpair");  //�����˷�����ԴΪ/hostpair��Get���������ӽ���ʧ�ܣ�Get�᷵��NULL
		if (rsp && rsp->status == 200) //�ж���Ӧ����Ƿ���ȷ 
		{
			host->_pair_ret = true;   //����������Խ��
		}
		return;
	}

	//1.��ȡ��������
	bool GetonlineHost()
	{
		char ch = 'Y'; //�Ƿ�����ƥ�䣬Ĭ���ǽ���ƥ��ģ����Ѿ�ƥ�����online������Ϊ�գ������û�ѡ��
		if (!_online_host.empty())
		{
			std::cout << "�Ƿ������������(Y/N)";
			fflush(stdout);
			std::cin >> ch;
		}
		if (ch == 'Y')
		{
			std::cout << "��ʼ����ƥ��...\n";
			//1.��ȡ������Ϣ������������о�����������IP��ַ�б���Ϣ
			std::vector<Adapter> list;
			AdapterUtil::GetAllAdapter(&list);

			//��ȡ��������IP��ַ����ӵ�host_list��
			std::vector<Host> host_list;
			for (int i = 0; i < list.size(); ++i)//�õ���������IP��ַ�б�
			{
				uint32_t ip = list[i]._ip_addr;
				uint32_t mask = list[i]._mask_addr;

				//���������
				uint32_t net = (ntohl(ip & mask));

				//�������������
				uint32_t max_host = (~ntohl(mask));
				for (int j = 1; j < (int32_t)max_host; ++j)
				{
					uint32_t host_ip = net + j;  //������IP��ַ������ź����������
					Host host;
					host._ip_addr = htonl(host_ip);//�������ֽ���ת��Ϊ�����ֽ���
					host._pair_ret = false;
					host_list.push_back(host);
				}
			}
			//��Host_list�����������߳̽�����ԣ�ȡָ������Ϊstd::thread��һ���ֲ�������Ϊ�˷�ֹ��ɺ��ͷ�
			std::vector<std::thread*> thr_list(host_list.size());
			for (int i = 0; i < host_list.size(); ++i)
			{
				thr_list[i] = new std::thread(&Client::HostPair, this, &host_list[i]);
			}
			std::cout << "��������ƥ���У����Ժ�....\n";
			//�ȴ����������߳������ϣ��ж���Խ����������������ӵ�_online_host��
			for (int i = 0; i < host_list.size(); ++i)
			{
				thr_list[i]->join();
				if (host_list[i]._pair_ret == true)
				{
					_online_host.push_back(host_list[i]);
				}
				delete thr_list[i];
			}
		}
		//��ӡ���������б����û�ѡ��
		for (int i = 0; i < _online_host.size(); ++i)
		{
			char buf[MAX_IPBUFFER] = { 0 };
			inet_ntop(AF_INET, &_online_host[i]._ip_addr, buf, MAX_IPBUFFER);
			std::cout << "\t" << buf << std::endl;
		}
		//2.�����IP��ַ�����������
		//3.���������õ���Ӧ�����Ӧ������������IP��ӵ�_online_host�б���
		std::cout << "��ѡ�������������ȡ�����ļ��б�" << std::endl;
		fflush(stdout);
		std::string select_ip;
		std::cin >> select_ip;
		GetShareList(select_ip); //�û�ѡ������֮�󣬵��û�ȡ�ļ��б�ӿ�
		return true;
	}

	//2.��ȡ�ļ��б�
	bool GetShareList(const std::string &host_ip)
	{
		//�����˷���һ���ļ��б��ȡ����
		//1.�ȷ�������
		//2.�õ���Ӧ֮�󣬽�������(�ļ�����)

		httplib::Client cli(host_ip.c_str(), P2P_PORT);
		auto rsp = cli.Get("st");
		if (rsp == NULL || rsp->status != 200)
		{
			std::cerr << "��ȡ�ļ��б���Ӧ����\n" << std::endl;
			return false;
		}

		//��ӡ����--����ӡ�������Ӧ���ļ������б����û�ѡ��
		//body:filename1\r\nfilename
		std::cout << rsp->body << std::endl;
		std::cout << "\n��ѡ��Ҫ���ص��ļ�";
		fflush(stdout);
		std::string filename;
		std::cin >> filename;
		DownloadFile(host_ip, filename);

		return true;
	}

	//3.�����ļ�
	bool DownloadFile(const std::string &host_ip, const std::string& filename)
	{
		//1.�����˷����ļ���������--��filename
		//2.�õ���Ӧ�������Ӧ�е�body���ľ����ļ�����
		//3.�����ļ������ļ�д���ļ��У��ر��ļ�
		std::string req_path = "/download/" + filename;
		httplib::Client cli(host_ip.c_str(), P2P_PORT);
		auto rsp = cli.Get(req_path.c_str());
		if (rsp == NULL || rsp->status != 200)
		{
			std::cerr << "�����ļ���ȡ��Ӧʧ��/n";
			return false;
		}
		std::cout << "��ȡ�����ļ���Ӧ�ɹ�\n";
		if (!boost::filesystem::exists(DOWNLOAD_PATH))
		{
			boost::filesystem::create_directory(DOWNLOAD_PATH);
		}
		std::string realpath = DOWNLOAD_PATH + '/' + filename;
		if (FileUtil::Write(realpath, rsp->body) == false)
		{
			std::cerr << "�ļ�����ʧ��\n";
			return false;
		}
		return true;
	}

private:
	std::vector<Host> _online_host;
};
//����˴���ģ��
class Server
{
public:
	bool Start()
	{
		//��ӶԿͻ�������Ĵ���ʽ��Ӧ��ϵ
		_srv.Get("/hostpair", HostPair);
		_srv.Get("st", ShareList);

		//������ʽ���������ַ���ָ���ĸ�ʽ����ʾ���йؼ�����������
		//������ʽ��.��ƥ���\r��\n֮��������ַ���*��ʾƥ��ǰ�ߵ��ַ������
		//��ֹ���Ϸ�����������ͻ������������м���download·��
		_srv.Get("/download/.*", Download);
		_srv.listen("0.0.0.0", P2P_PORT);
		return true;
	}

private:
	static void HostPair(const httplib::Request &req, httplib::Response &rsp)
	{
		rsp.status = 200;
		return;
	}

	//��ȡ�����ļ��б�������������һ������Ŀ¼���������Ŀ¼�µ��ļ�����Ҫ�����˹����
	static void ShareList(const httplib::Request &req, httplib::Response &rsp)
	{
		//1.�鿴Ŀ¼�Ƿ���ڣ��������򴴽�Ŀ¼
		if (!boost::filesystem::exists(SHARED_PATH))
		{
			boost::filesystem::create_directory(SHARED_PATH);
		}
		boost::filesystem::directory_iterator begin(SHARED_PATH);// ʵ����Ŀ¼������
		boost::filesystem::directory_iterator end;   //ʵ������������ĩβ
		//��ʼ����Ŀ¼
		for (; begin != end; ++begin)
		{
			if (boost::filesystem::is_directory(begin->status()))
			{
				//��ǰ�汾����ֻ��ȡ�ļ����ƣ�������㼶Ŀ¼����
				continue;
			}
			std::string name = begin->path().filename().string();  //ֻҪ�ļ�������Ҫ·��
			rsp.body += name + "\r\n";  //filename1\r\n filename2\r\n
		}
		rsp.status = 200;
		return;
	}

	static void Download(const httplib::Request &req, httplib::Response &rsp)
	{
		//req.path---�ͻ����������Դ·��   /download/filename.txt
		boost::filesystem::path req_path(req.path);
		std::string name = req_path.filename().string(); //ֻ��ȡ�ļ����ƣ�  filename.txt
		std::string realpath = SHARED_PATH + name;  //ʵ���ļ�·��Ӧ�����ڹ���Ŀ¼�µ�
		if (!boost::filesystem::exists(realpath) || boost::filesystem::is_directory(realpath))
		{
			rsp.status = 404;
			return;
		}
		if (FileUtil::Read(realpath, &rsp.body) == false)
		{
			rsp.status = 500;
			return;
		}
		rsp.status = 200;
		return;
	}
private:
	httplib::Server _srv;
};