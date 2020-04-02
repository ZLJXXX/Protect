#pragma once
#include<iostream>
#include<vector>
#include<fstream>
#include <boost/filesystem.hpp>
#ifdef _WIN32
//windowsͷ�ļ�
#include <WinSock2.h>
#include<Ws2tcpip.h>
#include<Iphlpapi.h>//��ȡ������Ϣ��ͷ�ļ�
#pragma comment(lib, "Iphlpapi.lib")//��ȡ������Ϣ�ӿڵĿ��ļ�
#pragma comment(lib, "ws2_32.lib")//windows�µ�socket��
#else
//linuxͷ�ļ�
#endif 
class FileUtil //�ļ������Ĺ�����
{
public:
	static bool Write(const std::string &name, const std::string &body,int offset = 0){//�ļ���ƫ����
		std::ofstream ofs(name);//�ļ������
		if (ofs.is_open() == false){
			std::cerr << "���ļ�ʧ��";
			return false;
		}
		//�ļ��Ѿ��򿪣�д������
		ofs.seekp(offset, std::ios::beg);//x������ļ���ʼ��ƫ��
		ofs.write(&body[0], body.size());
		if (ofs.good() == false){
			std::cerr << "���ļ�д������ʧ��\n";
			ofs.close();
			return false;
		}
		ofs.close();
		return true;
	}
	//ָ�������ʾ����һ������Ͳ���
	//const������ʾ����һ�������Ͳ���
	//& ��ʾ����һ����������Ͳ���
	static bool Read(const std::string &name, std::string *body){
		std::ifstream ifs(name);//������
		if (ifs.is_open() == false){
			std::cerr << "���ļ�ʧ��";
			return false;
		}
		int filesize = boost::filesystem::file_size(name);//��ȡ�ļ���С
		body->resize(filesize);
		ifs .read(&(*body)[0],filesize);
		if (ifs.good() == false){
			std::cerr << "��ȡ�ļ�����ʧ��\n";
			ifs.close();
			return false;
		}
		ifs.close();
		return true;
	}
};
//����������
class Adapter 
{
public:
	int _ip_addr; //�����ϵ�ip��ַ
	int _mask_addr;//�����ϵ���������
};
class AdapterUtil
{
public:
#ifdef _WIN32
	//windows�»�ȡ������Ϣʵ��
	static bool GetAllAdapter(std::vector<Adapter> *list){
		//IP_ADAPTER_INFO ���������Ϣ�Ľṹ��
		PIP_ADAPTER_INFO p_adapters = new IP_ADAPTER_INFO();
		//GetAdaptersInfo win�»�ȡ������Ϣ�Ľӿ� 
		//������Ϣ�����ж���������ָ��
		//��ȡ������Ϣ���ܻ�ʧ�ܣ��ռ䲻��)�������һ������Ͳ������������û���������������Ϣ�Ŀռ��С
		int all_adapter_size = sizeof(IP_ADAPTER_INFO);
		int ret = GetAdaptersInfo(p_adapters, (PULONG)&all_adapter_size);
		if (ret == ERROR_BUFFER_OVERFLOW){//������󣬱�ʾ�������ռ䲻�㣬���¸���ָ������ռ�
			delete p_adapters;
			//all_adapter_size���ڻ�ȡʵ������������Ϣռ�õĿռ�
			p_adapters = (PIP_ADAPTER_INFO)new BYTE[all_adapter_size];
			GetAdaptersInfo(p_adapters, (PULONG)&all_adapter_size);//���»�ȡ������Ϣ
		}
		while (p_adapters){
			Adapter adapter;
			inet_pton(AF_INET, p_adapters->IpAddressList.IpAddress.String, &adapter._ip_addr);
			inet_pton(AF_INET, p_adapters->IpAddressList.IpMask.String, &adapter._mask_addr);
			if (adapter._ip_addr != 0){//��Щ������û�����ã�����ip��ַΪ0
				list->push_back(adapter); //��������Ϣ��ӵ�vector���ظ��û�
				std::cout << "��������: " << p_adapters->AdapterName << std::endl;
				std::cout << "��������: " << p_adapters->Description << std::endl;
				std::cout << "IP��ַ: " << p_adapters->IpAddressList.IpAddress.String << std::endl;
				std::cout << "��������: " << p_adapters->IpAddressList.IpMask.String << std::endl;
			}	
			p_adapters = p_adapters->Next;
		}
		delete p_adapters;
		return true;
	}
#else
	//linux�»�ȡ������Ϣ�ӿ�
	bool GetAllAdapter(std::vector<Adapter> *list);
#endif
};