#pragma once
#include<thread>  //线程的头文件
#include"util.h"
#include"httplib.h"
#include <boost/filesystem.hpp>
#define P2P_PORT 9000 //900千端口
#define MAX_IPBUFFER 16  //转换后的ip地址放入缓冲区,缓冲区的大小
#define MAX_RANGE (100 * 1024 * 1024) //分块传输区间大小，100M
#define SHARED_PATH "./Shared/" //共享目录
#define DOWNLOAD_PATH "./download/"
class Host
{
public:
	int _ip_addr; //要配对的主机IP地址
	bool _pair_ret;//用于存放配对结果，配对成功 true
};
class Client
{
public:
	bool Start()
	{
		//客户端程序需要循环运行，因为下载文件不是只下再一次
		//循环下载每次下载一个文件之后都会进行主机配对并不合理
		while (1)
		{
			GetonlineHost();
		}
		return true;
	}
	//主机配对线程入口函数
	void HostPair(Host *host)
	{
		//1.组织HTTP协议格式的请求数据
		//2.搭建一个TCP客户端，将数据发送
		//3.等待服务器端的回复，并进行解析

		//这个过程使用第三方库httplib实现，注：此处须知httplib的总体上实现流程
		host->_pair_ret = false;
		char buf[MAX_IPBUFFER] = { 0 };
		inet_ntop(AF_INET, &host->_ip_addr, buf, MAX_IPBUFFER);
		httplib::Client cli(buf, P2P_PORT); //实例化httplib客户端对象
		auto rsp = cli.Get("/hostpair");  //向服务端发送资源为/hostpair的Get请求，若连接建立失败，Get会返回NULL
		if (rsp && rsp->status == 200) //判断响应结果是否正确 
		{
			host->_pair_ret = true;   //重置主机配对结果
		}
		return;
	}

	//1.获取在线主机
	bool GetonlineHost()
	{
		char ch = 'Y'; //是否重新匹配，默认是进行匹配的，若已经匹配过，online主机不为空，则让用户选择
		if (!_online_host.empty())
		{
			std::cout << "是否重新配对主机(Y/N)";
			fflush(stdout);
			std::cin >> ch;
		}
		if (ch == 'Y')
		{
			std::cout << "开始主机匹配...\n";
			//1.获取网卡信息，进而获得所有局域网中所有IP地址列表信息
			std::vector<Adapter> list;
			AdapterUtil::GetAllAdapter(&list);

			//获取所有主机IP地址，添加到host_list中
			std::vector<Host> host_list;
			for (int i = 0; i < list.size(); ++i)//得到所有主机IP地址列表
			{
				uint32_t ip = list[i]._ip_addr;
				uint32_t mask = list[i]._mask_addr;

				//计算网络号
				uint32_t net = (ntohl(ip & mask));

				//计算最大主机号
				uint32_t max_host = (~ntohl(mask));
				for (int j = 1; j < (int32_t)max_host; ++j)
				{
					uint32_t host_ip = net + j;  //主机的IP地址由网络号和主机号相加
					Host host;
					host._ip_addr = htonl(host_ip);//将主机字节序转换为网络字节序
					host._pair_ret = false;
					host_list.push_back(host);
				}
			}
			//对Host_list中主机创建线程进行配对，取指针是因为std::thread是一个局部变量，为了防止完成后被释放
			std::vector<std::thread*> thr_list(host_list.size());
			for (int i = 0; i < host_list.size(); ++i)
			{
				thr_list[i] = new std::thread(&Client::HostPair, this, &host_list[i]);
			}
			std::cout << "正在主机匹配中，请稍后....\n";
			//等待所有主机线程配对完毕，判断配对结果，将在线主机添加到_online_host中
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
		//打印在线主机列表，供用户选择
		for (int i = 0; i < _online_host.size(); ++i)
		{
			char buf[MAX_IPBUFFER] = { 0 };
			inet_ntop(AF_INET, &_online_host[i]._ip_addr, buf, MAX_IPBUFFER);
			std::cout << "\t" << buf << std::endl;
		}
		//2.逐个对IP地址表发送配对请求
		//3.若配对请求得到响应，则对应在线主机，则将IP添加到_online_host列表中
		std::cout << "请选择配对主机，获取共享文件列表" << std::endl;
		fflush(stdout);
		std::string select_ip;
		std::cin >> select_ip;
		GetShareList(select_ip); //用户选择主机之后，调用获取文件列表接口
		return true;
	}

	//2.获取文件列表
	bool GetShareList(const std::string &host_ip)
	{
		//向服务端发送一个文件列表获取请求
		//1.先发送请求
		//2.得到相应之后，解析正文(文件名称)

		httplib::Client cli(host_ip.c_str(), P2P_PORT);
		auto rsp = cli.Get("st");
		if (rsp == NULL || rsp->status != 200)
		{
			std::cerr << "获取文件列表响应错误\n";
			return false;
		}

		//打印正文--》打印服务端响应的文件名称列表，供用户选择
		//body:filename1\r\nfilename
		std::cout << rsp->body << std::endl;
		std::cout << "\n请选择要下载的文件";
		fflush(stdout);
		std::string filename;
		std::cin >> filename;
		RangeDownload(host_ip, filename);

		return true;
	}

	//3.下载文件
	bool DownloadFile(const std::string &host_ip, const std::string& filename)
	{
		//1.向服务端发送文件下载请求--》filename
		//2.得到响应结果，响应中的body正文就是文件数据
		//3.创建文件，将文件写入文件中，关闭文件
		std::string req_path = "/download/" + filename;
		httplib::Client cli(host_ip.c_str(), P2P_PORT);
		std::cout << "向服务端发送文件下载请求:" << host_ip << req_path << std::endl;
		auto rsp = cli.Get(req_path.c_str());
		if (rsp == NULL || rsp->status != 200)
		{
			std::cerr << "下载文件获取响应失败\n";
			return false;
		}
		std::cout << "获取下载文件响应成功\n";
		if (!boost::filesystem::exists(DOWNLOAD_PATH))
		{
			boost::filesystem::create_directory(DOWNLOAD_PATH);
		}
		std::string realpath = DOWNLOAD_PATH + filename;
		if (FileUtil::Write(realpath, rsp->body) == false)
		{
			std::cerr << "文件下载失败\n";
			return false;
		}
		std::cout << "文件下载成功\n";
		return true;
	}
	//实现分段传输，文件较大时，效率更高
	bool RangeDownload(const std::string &host_ip, const std::string &name){
		//1.发送Head请求，通过响应中的Content_Length获取文件大小	
		std::string req_path = "/download/" + name;
		httplib::Client cli(host_ip.c_str(), P2P_PORT);
		auto rsp = cli.Head(req_path.c_str());
		if (rsp == NULL || rsp->status != 200){
			std::cout << "获取文件大小失败\n";
			return false;
		}
		std::string clen = rsp->get_header_value("Content-Length");
		int filesize = StringUtil::Str2Dig(clen);

		//2.根据文件大小进行分块
		//int range_count = filesize / MAX_RANGE;
		//a.若文件大小小于块大小，则直接下载文件
		if (filesize < MAX_RANGE){
			std::cout << "文件较小，直接下载文件\n";
			return DownloadFile(host_ip, name);
		}
		//b.若文件大小不能整除块大小,则分块个数为文件大小除以块大小+1
		std::cout << "文件过大，分块进行下载\n";
		int range_count = 0;
		if (filesize % MAX_RANGE == 0){
			range_count = filesize / MAX_RANGE;
		}
		else{
			range_count = (filesize / MAX_RANGE) + 1;
		}
		int range_start = 0, range_end = 0;
		for (int i = 0; i < range_count; i++){
			range_start = i * MAX_RANGE;
			if (i == (range_count - 1)){//末尾分块
				range_end = filesize - 1;
			}
			else{
				range_end = ((i + 1) * MAX_RANGE) - 1;
			}
			std::cout << "客户端请求分块" << range_start << "-" << range_end << std::endl;
			//c.若文件大小刚好整除块大小，则分块个数为文件大小除以块大小
			//3.逐一请求分块区间的数据，得到响应后写入文件的指定位置
			httplib::Headers header;
			header.insert(httplib::make_range_header({ { range_start, range_end } }));//设置一个range区间
			httplib::Client cli(host_ip.c_str(), P2P_PORT);
			auto rsp = cli.Get(req_path.c_str(), header);//向服务端发送一个分段请求
			if (rsp == NULL || rsp->status != 206){
				std::cout << "区间下载文件失败\n";
				return false;
			}
			//获取文件成功，向文件中分块写入数据
			FileUtil::Write(name, rsp->body, range_start);
		}
		std::cout << "文件下载成功\n";
		return true;
	}
private:
	std::vector<Host> _online_host;
};
//服务端代码模块
class Server
{
public:
	bool Start()
	{
		//添加对客户端请求的处理方式对应关系
		_srv.Get("/hostpair", HostPair);
		_srv.Get("st", ShareList);

		//正则表达式：将特殊字符以指定的格式，表示具有关键特征的数据
		//正则表达式中.：匹配除\r或\n之外的任意字符，*表示匹配前边的字符任意次
		//防止与上方的请求发生冲突，因此在请求中加入download路径
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

	//获取共享文件列表，在主机上设置一个共享目录，凡是这个目录下的文件都是要给别人共享的
	static void ShareList(const httplib::Request &req, httplib::Response &rsp)
	{
		//1.查看目录是否存在，不存在则创建目录
		if (!boost::filesystem::exists(SHARED_PATH))
		{
			boost::filesystem::create_directory(SHARED_PATH);
		}
		boost::filesystem::directory_iterator begin(SHARED_PATH);// 实例化目录迭代器
		boost::filesystem::directory_iterator end;   //实例化迭代器的末尾
		//开始迭代目录
		for (; begin != end; ++begin)
		{
			if (boost::filesystem::is_directory(begin->status()))
			{
				//当前版本我们只获取文件名称，不做多层级目录操作
				continue;
			}
			std::string name = begin->path().filename().string();  //只要文件名，不要路径
			rsp.body += name + "\r\n";  //filename1\r\n filename2\r\n
		}
		rsp.status = 200;
		return;
	}

	static void Download(const httplib::Request &req, httplib::Response &rsp)
	{
		std::cout << "服务端收到文件下载请求:" << req.path << std::endl;
		//req.path---客户端请求的资源路径   /download/filename.txt
		boost::filesystem::path req_path(req.path);
		std::string name = req_path.filename().string(); //只获取文件名称；  filename.txt
		std::cout << "服务端收到实际的文件下载名称:" << name << std::endl;
		std::string realpath = SHARED_PATH + name;  //实际文件路径应该是在共享目录下的
		std::cout << "服务端收到实际的文件下载路径:" << realpath << std::endl;
		if (!boost::filesystem::exists(realpath) || boost::filesystem::is_directory(realpath))
		{
			rsp.status = 404;
			return;
		}
		if (req.method == "GET"){
			//判断是否是分块传输的依据，就是这次请求中是否有range头部字段
			if (req.has_header("Range")){//判断请求头中是否包含Range字段
				//这就是一个分块传输
				//需要知道分块区间是多少
				std::string range_str = req.get_header_value("Range");
				httplib::Ranges ranges;//vector
				httplib::detail::parse_range_header(range_str, ranges);
				int range_start = ranges[0].first;
				int range_end = ranges[0].second;
				int range_len = range_end - range_start + 1;
				std::cout << "range:" << range_start << "-" << range_end << std::endl;
				FileUtil::ReadRange(realpath, &rsp.body, range_len, range_start);
				rsp.status = 206;
				std::cout << "服务端响应区间完毕\n";
			}
			else{
				//没有Range头部，则是一个完成的文件下载
				if (FileUtil::Read(realpath, &rsp.body) == false)
				{
					rsp.status = 500;
					return;
				}
				rsp.status = 200;
			}
		}
		else{
			//这个是针对HEAD请求的客户端只要头部不要正文
			int filesize = FileUtil::GetFileSize(realpath);
			rsp.set_header("Content-Length", std::to_string(filesize));//设置响应header头部信息接口
			rsp.status = 200;
		}
		return;
	}
private:
	httplib::Server _srv;
};