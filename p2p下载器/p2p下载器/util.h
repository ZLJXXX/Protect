#pragma once
#include<iostream>
#include<vector>
#include<fstream>
#include<sstream>
#include <boost/filesystem.hpp>
#ifdef _WIN32
//windows头文件
#include <WinSock2.h>
#include<Ws2tcpip.h>
#include<Iphlpapi.h>//获取网卡信息的头文件
#pragma comment(lib, "Iphlpapi.lib")//获取网卡信息接口的库文件
#pragma comment(lib, "ws2_32.lib")//windows下的socket库
#else
//linux头文件
#endif 
class StringUtil{
public:
	static int Str2Dig(const std::string &num){
		std::stringstream tmp;
		tmp << num;
		int res;
		tmp >> res;
		return res;
	}
};
class FileUtil //文件操作的工具类
{
public:
	static int GetFileSize(const std::string &name){
		return boost::filesystem::file_size(name);
	}
	static bool Write(const std::string &name, const std::string &body,int offset = 0){//文件的偏移量
		//采用二进制的方法写入
		FILE *fp = NULL;
		fopen_s(&fp, name.c_str(), "wb+");
		if (fp == NULL){
			std::cerr << "打开文件失败\n";
			return false;
		}
		fseek(fp, offset, SEEK_SET);
		int ret = fwrite(body.c_str(), 1, body.size(), fp);
		if (ret != body.size()){
			std::cerr << "向文件写入数据失败\n";
			fclose(fp);
			return false;
		}
		fclose(fp);
		return true;
		//c++方式文件下载，数据写入时，其编译方法不同，导致文件下载后大小不同
		/*std::ofstream ofs(name);//文件输出流
		if (ofs.is_open() == false){
			std::cerr << "打开文件失败";
			return false;
		}
		//文件已经打开，写入数据
		ofs.seekp(offset, std::ios::beg);//x相对于文件开始处偏移
		ofs.write(&body[0], body.size());
		if (ofs.good() == false){
			std::cerr << "向文件写入数据失败\n";
			ofs.close();
			return false; 
		}
		ofs.close();*/
	}
	//指针参数表示其是一个输出型参数
	//const参数表示其是一个输入型参数
	//& 表示这是一个输入输出型参数
	static bool Read(const std::string &name, std::string *body){
		//二进制方式读取
		int filesize = boost::filesystem::file_size(name);
		body->resize(filesize);
		std::cout << "读取文件数据:" << name << "size:" << filesize << "\n";
		FILE *fp = NULL;
		fopen_s(&fp, name.c_str(), "rb+");
		if (fp == NULL){
			std::cerr << "打开文件失败\n";
			return false;
		}
		int ret = fread(&(*body)[0], 1, filesize, fp);
		if (ret != filesize){
			std::cerr << "从文件读取数据失败\n";
			fclose(fp);
			return false;
		}
		fclose(fp);
		return true;
		/*std::ifstream ifs(name);//输入流
		if (ifs.is_open() == false){
			std::cerr << "打开文件失败";
			return false;
		}
		int filesize = boost::filesystem::file_size(name);//获取文件大小
		body->resize(filesize);
		std::cout << "要读取的文件大小:" << name << ":" << filesize << std::endl;
		ifs .read(&(*body)[0],filesize);
		///*
		//if (ifs.good() == false){
		//	std::cerr << "读取文件数据失败\n";
		//	ifs.close();
		//	return false;
		//}
		ifs.close();
		return true;
		*/
	}
	static bool ReadRange(const std::string &name, std::string *body, int len, int offset){
		body->resize(len);
		FILE *fp = NULL;
		fopen_s(&fp, name.c_str(), "rb+");
		if (fp == NULL){
			std::cerr << "打开文件失败\n";
			return false;
		}
		fseek(fp, offset, SEEK_SET);
		int ret = fread(&(*body)[0], 1, len, fp);
		if (ret != len){
			std::cerr << "从文件读取数据失败\n";
			fclose(fp);
			return false;
		}
		fclose(fp);
		return true;
	}
};
//网络适配器
class Adapter 
{
public:
	int _ip_addr; //网卡上的ip地址
	int _mask_addr;//网卡上的子网掩码
};
class AdapterUtil
{
public:
#ifdef _WIN32
	//windows下获取网卡信息实现
	static bool GetAllAdapter(std::vector<Adapter> *list){
		//IP_ADAPTER_INFO 存放网卡信息的结构体
		PIP_ADAPTER_INFO p_adapters = new IP_ADAPTER_INFO();
		//GetAdaptersInfo win下获取网卡信息的接口 
		//网卡信息可能有多个，因此用指针
		//获取网卡信息可能会失败（空间不足)，因此有一个输出型参数，用于向用户返回所有网卡信息的空间大小
		int all_adapter_size = sizeof(IP_ADAPTER_INFO);
		int ret = GetAdaptersInfo(p_adapters, (PULONG)&all_adapter_size);
		if (ret == ERROR_BUFFER_OVERFLOW){//这个错误，表示缓冲区空间不足，重新给这指针申请空间
			delete p_adapters;
			//all_adapter_size用于获取实际所有网卡信息占用的空间
			p_adapters = (PIP_ADAPTER_INFO)new BYTE[all_adapter_size];
			GetAdaptersInfo(p_adapters, (PULONG)&all_adapter_size);//重新获取网卡信息
		}
		while (p_adapters){
			Adapter adapter;
			inet_pton(AF_INET, p_adapters->IpAddressList.IpAddress.String, &adapter._ip_addr);
			inet_pton(AF_INET, p_adapters->IpAddressList.IpMask.String, &adapter._mask_addr);
			if (adapter._ip_addr != 0){//有些网卡并没有启用，导致ip地址为0
				list->push_back(adapter); //将网卡信息添加到vector返回给用户
				std::cout << "网卡名称: " << p_adapters->AdapterName << std::endl;
				std::cout << "网卡描述: " << p_adapters->Description << std::endl;
				std::cout << "IP地址: " << p_adapters->IpAddressList.IpAddress.String << std::endl;
				std::cout << "子网掩码: " << p_adapters->IpAddressList.IpMask.String << std::endl;
			}	
			p_adapters = p_adapters->Next;
		}
		delete p_adapters;
		return true;
	}
#else
	//linux下获取网卡信息接口
	bool GetAllAdapter(std::vector<Adapter> *list);
#endif
};