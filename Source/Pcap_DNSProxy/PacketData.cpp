﻿// This code is part of Pcap_DNSProxy
// Pcap_DNSProxy, a local DNS server based on WinPcap and LibPcap
// Copyright (C) 2012-2017 Chengr28
// 
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.


#include "PacketData.h"

/* Get Ethernet Frame Check Sequence/FCS
uint32_t GetFCS(
	const uint8_t *Buffer, 
	const size_t Length)
{
	uint32_t Table[FCS_TABLE_SIZE]{0}, Gx = 0x04C11DB7, Temp = 0, CRC_Table = 0, Value = 0, UI = 0;
	uint8_t ReflectNum[]{8, 32};
	int Index[]{0, 0, 0};
	for (Index[0] = 0;Index[0] <= UINT8_MAX;++Index[0])
	{
		Value = 0;
		UI = Index[0];

		for (Index[1U] = 1;Index[1U] < 9;++Index[1U])
		{
			if (UI & 1)
				Value |= 1 << (ReflectNum[0] - Index[1U]);
			UI >>= 1;
		}

		Temp = Value;
		Table[Index[0]] = Temp << 24U;

		for (Index[2U] = 0;Index[2U] < 8;++Index[2U])
		{
			unsigned long int t1 = 0, t2 = 0, Flag = Table[Index[0]] & 0x80000000;
			t1 = (Table[Index[0]] << 1);
			if (Flag == 0)
				t2 = 0;
			else 
				t2 = Gx;
			Table[Index[0]] = t1 ^ t2;
		}

		CRC_Table = Table[Index[0]];
		UI = Table[Index[0]];
		Value = 0;

		for (Index[1U] = 1;Index[1U] < 33;++Index[1U])
		{
			if (UI & 1)
				Value |= 1 << (ReflectNum[1U] - Index[1U]);
			UI >>= 1;
		}

		Table[Index[0]] = Value;
	}

	uint32_t CRC = UINT32_MAX;
	for (Index[0] = 0;Index[0] < static_cast<int>(Length);+Index[0])
		CRC = Table[(CRC ^ (*(Buffer + Index[0]))) & UINT8_MAX] ^ (CRC >> (sizeof(uint8_t) * BYTES_TO_BITS));

	return ~CRC;
}
*/

//Get checksum
uint16_t GetChecksum(
	const uint16_t *Buffer, 
	const size_t Length)
{
	uint32_t Result = CHECKSUM_SUCCESS;
	auto InnerLength = Length;
	while (InnerLength > 1U)
	{
		Result += *Buffer++;
		InnerLength -= sizeof(uint16_t);
	}

	if (InnerLength)
		Result += *reinterpret_cast<const uint8_t *>(Buffer);
	Result = (Result >> (sizeof(uint16_t) * BYTES_TO_BITS)) + (Result & UINT16_MAX);
	Result += (Result >> (sizeof(uint16_t) * BYTES_TO_BITS));

	return static_cast<uint16_t>(~Result);
}

//Get ICMPv6 checksum
uint16_t GetChecksum_ICMPv6(
	const uint8_t * const Buffer, 
	const size_t Length, 
	const in6_addr &Destination, 
	const in6_addr &Source)
{
//Initialization
	std::unique_ptr<uint8_t[]> Validation(new uint8_t[sizeof(ipv6_psd_hdr) + Length]());
	memset(Validation.get(), 0, sizeof(ipv6_psd_hdr) + Length);

//Get checksum.
	reinterpret_cast<ipv6_psd_hdr *>(Validation.get())->Destination = Destination;
	reinterpret_cast<ipv6_psd_hdr *>(Validation.get())->Source = Source;
	reinterpret_cast<ipv6_psd_hdr *>(Validation.get())->Length = htonl(static_cast<uint32_t>(Length));
	reinterpret_cast<ipv6_psd_hdr *>(Validation.get())->NextHeader = IPPROTO_ICMPV6;
	memcpy_s(Validation.get() + sizeof(ipv6_psd_hdr), Length, Buffer + sizeof(ipv6_hdr), Length);

	return GetChecksum(reinterpret_cast<uint16_t *>(Validation.get()), sizeof(ipv6_psd_hdr) + Length);
}

//Get TCP or UDP checksum
uint16_t GetChecksum_TCP_UDP(
	const uint16_t Protocol_Network, 
	const uint16_t Protocol_Transport, 
	const uint8_t * const Buffer, 
	const size_t Length)
{
	if (Protocol_Network == AF_INET6)
	{
		std::unique_ptr<uint8_t[]> Validation(new uint8_t[sizeof(ipv6_psd_hdr) + Length]());
		memset(Validation.get(), 0, sizeof(ipv6_psd_hdr) + Length);
		reinterpret_cast<ipv6_psd_hdr *>(Validation.get())->Destination = reinterpret_cast<const ipv6_hdr *>(Buffer)->Destination;
		reinterpret_cast<ipv6_psd_hdr *>(Validation.get())->Source = reinterpret_cast<const ipv6_hdr *>(Buffer)->Source;
		reinterpret_cast<ipv6_psd_hdr *>(Validation.get())->Length = htonl(static_cast<uint32_t>(Length));
		reinterpret_cast<ipv6_psd_hdr *>(Validation.get())->NextHeader = static_cast<uint8_t>(Protocol_Transport);
		memcpy_s(Validation.get() + sizeof(ipv6_psd_hdr), Length, Buffer + sizeof(ipv6_hdr), Length);

		return GetChecksum(reinterpret_cast<uint16_t *>(Validation.get()), sizeof(ipv6_psd_hdr) + Length);
	}
	else if (Protocol_Network == AF_INET)
	{
		std::unique_ptr<uint8_t[]> Validation(new uint8_t[sizeof(ipv4_psd_hdr) + Length]());
		memset(Validation.get(), 0, sizeof(ipv4_psd_hdr) + Length);
		reinterpret_cast<ipv4_psd_hdr *>(Validation.get())->Destination = reinterpret_cast<const ipv4_hdr *>(Buffer)->Destination;
		reinterpret_cast<ipv4_psd_hdr *>(Validation.get())->Source = reinterpret_cast<const ipv4_hdr *>(Buffer)->Source;
		reinterpret_cast<ipv4_psd_hdr *>(Validation.get())->Length = htons(static_cast<uint16_t>(Length));
		reinterpret_cast<ipv4_psd_hdr *>(Validation.get())->Protocol = static_cast<uint8_t>(Protocol_Transport);
		memcpy_s(Validation.get() + sizeof(ipv4_psd_hdr), Length, Buffer + reinterpret_cast<const ipv4_hdr *>(Buffer)->IHL * IPV4_IHL_BYTES_TIMES, Length);

		return GetChecksum(reinterpret_cast<uint16_t *>(Validation.get()), sizeof(ipv4_psd_hdr) + Length);
	}

	return EXIT_FAILURE;
}

//Add length data to TCP DNS transmission
size_t AddLengthDataToHeader(
	uint8_t * const Buffer, 
	const size_t RecvLen, 
	const size_t MaxLen)
{
	if (RecvLen + sizeof(uint16_t) < MaxLen)
	{
		memmove_s(Buffer + sizeof(uint16_t), MaxLen - sizeof(uint16_t), Buffer, RecvLen);
		reinterpret_cast<dns_tcp_hdr *>(Buffer)->Length = htons(static_cast<uint16_t>(RecvLen));
		return RecvLen + sizeof(uint16_t);
	}

	return EXIT_FAILURE;
}

//Convert data from string to DNS query
size_t StringToPacketQuery(
	const uint8_t * const FName, 
	uint8_t * const TName)
{
//Initialization
	int Index[]{static_cast<int>(strnlen_s(reinterpret_cast<const char *>(FName), DOMAIN_MAXSIZE)), 0, 0};
	if (Index[0] > 0)
		--Index[0];
	else 
		return 0;
	Index[2U] = Index[0] + 1;
	*(TName + Index[0] + 2) = 0;

//Convert domain.
	for (;Index[0] >= 0;--Index[0], --Index[2U])
	{
		if (FName[Index[0]] == ASCII_PERIOD)
		{
			*(TName + Index[2U]) = static_cast<uint8_t>(Index[1U]);
			Index[1U] = 0;
		}
		else {
			*(TName + Index[2U]) = FName[Index[0]];
			++Index[1U];
		}
	}

	*(TName + Index[2U]) = static_cast<uint8_t>(Index[1U]);
	return strnlen_s(reinterpret_cast<const char *>(TName), DOMAIN_MAXSIZE - 1U) + NULL_TERMINATE_LENGTH;
}

//Convert data from DNS query to string
size_t PacketQueryToString(
	const uint8_t * const TName, 
	std::string &FName)
{
//Initialization
	size_t LocateIndex = 0;
	uint8_t StringIter[]{0, 0};
	int MarkIndex[]{0, 0};
	FName.clear();

//Convert domain.
	for (LocateIndex = 0;LocateIndex < DOMAIN_MAXSIZE;++LocateIndex)
	{
	//Domain pointer check
		if (TName[LocateIndex] >= DNS_POINTER_8_BITS)
		{
			return LocateIndex + sizeof(uint16_t);
		}
		else if (LocateIndex == 0)
		{
			MarkIndex[0] = TName[LocateIndex];
		}
		else if (LocateIndex == MarkIndex[0] + MarkIndex[1U] + 1U)
		{
			MarkIndex[0] = TName[LocateIndex];
			if (MarkIndex[0] == 0)
				break;

			MarkIndex[1U] = static_cast<int>(LocateIndex);
			FName.append(".");
		}
		else {
			StringIter[0] = TName[LocateIndex];
			FName.append(reinterpret_cast<const char *>(StringIter));
		}
	}

	return LocateIndex;
}

//Convert data from compression DNS query to whole DNS query
size_t MarkWholePacketQuery(
	const uint8_t * const WholePacket, 
	const size_t Length, 
	const uint8_t * const TName, 
	const size_t TNameIndex, 
	std::string &FName)
{
//Length and pointer index check
	if (FName.length() >= DOMAIN_MAXSIZE || TNameIndex < sizeof(dns_hdr) || TNameIndex >= Length)
		return 0;

//Initialization
	size_t LocateIndex = 0;
	uint8_t StringIter[]{0, 0};
	int Index[]{0, 0};

//Convert domain.
	for (LocateIndex = 0;LocateIndex < Length - TNameIndex;++LocateIndex)
	{
	//Domain pointer check
		if (TName[LocateIndex] >= DNS_POINTER_8_BITS)
		{
			const size_t PointerIndex = ntohs(*reinterpret_cast<const uint16_t *>(TName + LocateIndex)) & DNS_POINTER_BITS_GET_LOCATE;
			if (PointerIndex < TNameIndex)
			{
				if (!FName.empty())
					FName.append(".");

				return MarkWholePacketQuery(WholePacket, Length, WholePacket + PointerIndex, PointerIndex, FName);
			}
			else {
				return LocateIndex;
			}
		}
		else if (LocateIndex == 0)
		{
			Index[0] = TName[LocateIndex];
		}
		else if (LocateIndex == Index[0] + Index[1U] + 1U)
		{
			Index[0] = TName[LocateIndex];
			if (Index[0] == 0)
				break;
			else 
				Index[1U] = static_cast<int>(LocateIndex);

			FName.append(".");
		}
		else {
			StringIter[0] = TName[LocateIndex];
			FName.append(reinterpret_cast<const char *>(StringIter));
		}
	}

	return LocateIndex;
}

//Make ramdom domains
void MakeRamdomDomain(
	uint8_t * const Buffer)
{
//Ramdom number distribution initialization and make ramdom domain length.
	std::uniform_int_distribution<size_t> RamdomDistribution(DOMAIN_RAMDOM_MINSIZE, DOMAIN_LEVEL_DATA_MAXSIZE);
	auto RamdomLength = RamdomDistribution(*GlobalRunningStatus.RamdomEngine);
	if (RamdomLength < DOMAIN_RAMDOM_MINSIZE)
		RamdomLength = DOMAIN_RAMDOM_MINSIZE;
	size_t Index = 0;

//Make ramdom domain.
	if (RamdomLength % 2U == 0)
	{
		for (Index = 0;Index < RamdomLength - 3U;++Index)
		{
			*(Buffer + Index) = *(GlobalRunningStatus.DomainTable + RamdomDistribution(*GlobalRunningStatus.RamdomEngine));
			*(Buffer + Index) = static_cast<uint8_t>(tolower(*(Buffer + Index)));
		}

	//Make random domain like a normal Top-Level Domain/TLD.
		*(Buffer + (RamdomLength - 3U)) = ASCII_PERIOD;
		Index = RamdomDistribution(*GlobalRunningStatus.RamdomEngine);
		if (Index < ASCII_FF)
			Index += 52U;
		else if (Index < ASCII_AMPERSAND)
			Index += 26U;
		*(Buffer + (RamdomLength - 2U)) = *(GlobalRunningStatus.DomainTable + Index);
		Index = RamdomDistribution(*GlobalRunningStatus.RamdomEngine);
		if (Index < ASCII_FF)
			Index += 52U;
		else if (Index < ASCII_AMPERSAND)
			Index += 26U;
		*(Buffer + (RamdomLength - 1U)) = *(GlobalRunningStatus.DomainTable + Index);
	}
	else {
		for (Index = 0;Index < RamdomLength - 4U;++Index)
		{
			*(Buffer + Index) = *(GlobalRunningStatus.DomainTable + RamdomDistribution(*GlobalRunningStatus.RamdomEngine));
			*(Buffer + Index) = static_cast<uint8_t>(tolower(*(Buffer + Index)));
		}

	//Make random domain like a normal Top-level domain/TLD.
		*(Buffer + (RamdomLength - 4U)) = ASCII_PERIOD;
		Index = RamdomDistribution(*GlobalRunningStatus.RamdomEngine);
		if (Index < ASCII_FF)
			Index += 52U;
		else if (Index < ASCII_AMPERSAND)
			Index += 26U;
		*(Buffer + (RamdomLength - 3U)) = *(GlobalRunningStatus.DomainTable + Index);
		Index = RamdomDistribution(*GlobalRunningStatus.RamdomEngine);
		if (Index < ASCII_FF)
			Index += 52U;
		else if (Index < ASCII_AMPERSAND)
			Index += 26U;
		*(Buffer + (RamdomLength - 2U)) = *(GlobalRunningStatus.DomainTable + Index);
		Index = RamdomDistribution(*GlobalRunningStatus.RamdomEngine);
		if (Index < ASCII_FF)
			Index += 52U;
		else if (Index < ASCII_AMPERSAND)
			Index += 26U;
		*(Buffer + (RamdomLength - 1U)) = *(GlobalRunningStatus.DomainTable + Index);
	}

	return;
}

//Make Domain Case Conversion
void MakeDomainCaseConversion(
	uint8_t * const Buffer)
{
//Initialization
	auto Length = strnlen_s(reinterpret_cast<const char *>(Buffer), DOMAIN_MAXSIZE);
	if (Length <= DOMAIN_MINSIZE)
		return;
	std::vector<size_t> RamdomIndex;
	for (size_t Index = 0;Index < Length;++Index)
	{
		if (*(Buffer + Index) >= ASCII_LOWERCASE_A && *(Buffer + Index) <= ASCII_LOWERCASE_Z)
			RamdomIndex.push_back(Index);
	}

//Ramdom number distribution initialization
	if (RamdomIndex.empty())
		return;
	std::uniform_int_distribution<size_t> RamdomDistribution(0, RamdomIndex.size() - 1U);
	auto RamdomCounts = RamdomDistribution(*GlobalRunningStatus.RamdomEngine);
	if (RamdomCounts == 0)
		++RamdomCounts;

//Make Domain Case Conversion.
	for (size_t Index = 0;Index < RamdomCounts;++Index)
	{
		size_t BufferIndex = RamdomDistribution(*GlobalRunningStatus.RamdomEngine);
		*(Buffer + RamdomIndex.at(BufferIndex)) = static_cast<uint8_t>(toupper(*(Buffer + RamdomIndex.at(BufferIndex))));
	}

//Make sure that domain must have more than one char which in the last or the second last to convert.
	if (*(Buffer + (Length - 1U)) >= ASCII_LOWERCASE_A && *(Buffer + (Length - 1U)) <= ASCII_LOWERCASE_Z && 
		*(Buffer + (Length - 2U)) >= ASCII_LOWERCASE_A && *(Buffer + (Length - 2U)) <= ASCII_LOWERCASE_Z)
	{
		if (RamdomCounts % 2U == 0)
			*(Buffer + (Length - 1U)) = static_cast<uint8_t>(toupper(*(Buffer + (Length - 1U))));
		else 
			*(Buffer + (Length - 2U)) = static_cast<uint8_t>(toupper(*(Buffer + (Length - 2U))));
	}

	return;
}

//Move any EDNS Label to the end of packet
//Some records like SIG and TSIG need to locate at the end of the DNS packet, but they are only used to transport between DNS servers.
bool Move_EDNS_LabelToEnd(
	DNS_PACKET_DATA * const PacketStructure)
{
//Packet without EDNS Label
	if (PacketStructure->EDNS_Location == 0)
		return true;

//Packet with EDNS Label, but it's already stored at the end of packet.
	if (PacketStructure->EDNS_Location == PacketStructure->Records_Location.back())
	{
	//EDNS version check
		if (reinterpret_cast<edns_header *>(PacketStructure->Buffer + PacketStructure->EDNS_Location)->Version != EDNS_VERSION_ZERO)
			return false;
		else 
			return true;
	}

//Packet with EDNS Label, but it's not stored at the end of packet.
	for (size_t Index = 0;Index < PacketStructure->Records_Location.size();++Index)
	{
		if (PacketStructure->Records_Location.at(Index) == PacketStructure->EDNS_Location)
		{
		//EDNS version check
			if (reinterpret_cast<edns_header *>(PacketStructure->Buffer + PacketStructure->EDNS_Location)->Version != EDNS_VERSION_ZERO)
				return false;

		//Initialization
			std::unique_ptr<uint8_t[]> BufferTemp(new uint8_t[PacketStructure->Length + PADDING_RESERVED_BYTES]());
			memset(BufferTemp.get(), 0, PacketStructure->Length + PADDING_RESERVED_BYTES);

		//Copy all resource records except EDNS Label and copy EDNS Label to the end of packet.
			memcpy_s(BufferTemp.get(), PacketStructure->Length, PacketStructure->Buffer, PacketStructure->EDNS_Location);
			memcpy_s(BufferTemp.get() + PacketStructure->EDNS_Location, PacketStructure->Length - PacketStructure->EDNS_Location, PacketStructure->Buffer + PacketStructure->EDNS_Location + PacketStructure->EDNS_Length, PacketStructure->Length - PacketStructure->EDNS_Location - PacketStructure->EDNS_Length);
			memcpy_s(BufferTemp.get() + PacketStructure->Length - PacketStructure->EDNS_Length, PacketStructure->EDNS_Length, PacketStructure->Buffer + PacketStructure->EDNS_Location, PacketStructure->EDNS_Length);

		//Rebuild DNS counts.
			if (Index < PacketStructure->Records_AnswerCount)
				--PacketStructure->Records_AnswerCount;
			else if (Index < PacketStructure->Records_AnswerCount + PacketStructure->Records_AuthorityCount)
				--PacketStructure->Records_AuthorityCount;
			else if (Index < PacketStructure->Records_AnswerCount + PacketStructure->Records_AuthorityCount + PacketStructure->Records_AdditionalCount)
				--PacketStructure->Records_AdditionalCount;
			else 
				return true;
			++PacketStructure->Records_AdditionalCount;

		//Rebuild DNS header counts.
			reinterpret_cast<dns_hdr *>(BufferTemp.get())->Answer = htons(static_cast<uint16_t>(PacketStructure->Records_AnswerCount));
			reinterpret_cast<dns_hdr *>(BufferTemp.get())->Authority = htons(static_cast<uint16_t>(PacketStructure->Records_AuthorityCount));
			reinterpret_cast<dns_hdr *>(BufferTemp.get())->Additional = htons(static_cast<uint16_t>(PacketStructure->Records_AdditionalCount));

		//Copy back DNS packet.
			memset(PacketStructure->Buffer, 0, PacketStructure->Length);
			memcpy_s(PacketStructure->Buffer, PacketStructure->Length, BufferTemp.get(), PacketStructure->Length);

		//Move EDNS Label item.
			for (size_t InnerIndex = Index;InnerIndex < PacketStructure->Records_Length.size();++InnerIndex)
				PacketStructure->Records_Length.at(InnerIndex) -= PacketStructure->EDNS_Length;
			PacketStructure->Records_Location.erase(PacketStructure->Records_Location.begin() + Index);
			PacketStructure->Records_Length.erase(PacketStructure->Records_Length.begin() + Index);
			PacketStructure->Records_Location.push_back(PacketStructure->Length - PacketStructure->EDNS_Length);
			PacketStructure->Records_Length.push_back(PacketStructure->EDNS_Length);
			PacketStructure->EDNS_Location = PacketStructure->Length - PacketStructure->EDNS_Length;

			break;
		}
	}

	return true;
}

//Add EDNS label to DNS packet(C-Style string)
size_t Add_EDNS_LabelToPacket(
	uint8_t * const Buffer, 
	const size_t Length, 
	const size_t MaxLen, 
	const SOCKET_DATA * const LocalSocketData)
{
//Initialization
	const auto DNS_Header = reinterpret_cast<dns_hdr *>(Buffer);
	if (DNS_Header->Additional > 0)
		return Length;
	else 
		DNS_Header->Additional = htons(UINT16_NUM_ONE);
	auto DataLength = Length;

//Add a new EDNS/OPT Additional Resource Records.
	if (DataLength + sizeof(edns_header) >= MaxLen)
		return DataLength;
	const auto DNS_Record_OPT = reinterpret_cast<edns_header *>(Buffer + DataLength);
	DNS_Record_OPT->Type = htons(DNS_TYPE_OPT);
	DNS_Record_OPT->UDP_PayloadSize = htons(static_cast<uint16_t>(Parameter.EDNS_PayloadSize));
	DataLength += sizeof(edns_header);

//DNSSEC request
	if (Parameter.DNSSEC_Request)
	{
		DNS_Header->Flags = htons(ntohs(DNS_Header->Flags) | DNS_FLAG_GET_BIT_AD); //Set Authentic Data bit.
//		DNS_Header->Flags = htons(ntohs(DNS_Header->Flags) | DNS_FLAG_GET_BIT_CD); //Set Checking Disabled bit.
		DNS_Record_OPT->Z_Field = htons(ntohs(DNS_Record_OPT->Z_Field) | EDNS_FLAG_GET_BIT_DO); //Set Accepts DNSSEC security Resource Records bit.
	}

//EDNS client subnet
	if ((Parameter.EDNS_ClientSubnet_Relay && LocalSocketData != nullptr) || 
		Parameter.LocalMachineSubnet_IPv6 != nullptr || Parameter.LocalMachineSubnet_IPv4 != nullptr)
	{
		const auto DNS_Query = reinterpret_cast<dns_qry *>(Buffer + DNS_PACKET_QUERY_LOCATE(Buffer));

	//Length, DNS Class and DNS record check
		if (DataLength + sizeof(edns_client_subnet) >= MaxLen || ntohs(DNS_Query->Classes) != DNS_CLASS_INTERNET || 
			(ntohs(DNS_Query->Type) != DNS_TYPE_AAAA && ntohs(DNS_Query->Type) != DNS_TYPE_A))
				return DataLength;

		const auto EDNS_Subnet_Header = reinterpret_cast<edns_client_subnet *>(Buffer + DataLength);

	//AAAA record(IPv6)
		if (ntohs(DNS_Query->Type) == DNS_TYPE_AAAA && 
			((Parameter.EDNS_ClientSubnet_Relay && LocalSocketData != nullptr && LocalSocketData->SockAddr.ss_family == AF_INET6) || 
			Parameter.LocalMachineSubnet_IPv6 != nullptr))
		{
		//Make EDNS Subnet header.
			EDNS_Subnet_Header->Code = htons(EDNS_CODE_CSUBNET);
			EDNS_Subnet_Header->Family = htons(EDNS_ADDRESS_FAMILY_IPV6);

		//Mark network prefix.
			in6_addr BinaryAddr;
			memset(&BinaryAddr, 0, sizeof(BinaryAddr));
			if (Parameter.EDNS_ClientSubnet_Relay && LocalSocketData != nullptr && LocalSocketData->SockAddr.ss_family == AF_INET6)
			{
			//Default prefix of IPv6 address, please visit RFC 7871(https://tools.ietf.org/html/rfc7871).
				if (Parameter.LocalMachineSubnet_IPv6 != nullptr)
					EDNS_Subnet_Header->Netmask_Source = static_cast<uint8_t>(Parameter.LocalMachineSubnet_IPv6->second);
				else 
					EDNS_Subnet_Header->Netmask_Source = EDNS_CLIENT_SUBNET_SOURCE_PREFIX_IPV6;

			//Keep bits of address.
				BinaryAddr = reinterpret_cast<const sockaddr_in6 *>(&LocalSocketData->SockAddr)->sin6_addr;
				if (EDNS_Subnet_Header->Netmask_Source < sizeof(in6_addr) * BYTES_TO_BITS / 2U)
				{
					*reinterpret_cast<uint64_t *>(&BinaryAddr) = hton64(ntoh64(*reinterpret_cast<uint64_t *>(&BinaryAddr)) & (UINT64_MAX << (sizeof(in6_addr) * BYTES_TO_BITS / 2U - EDNS_Subnet_Header->Netmask_Source))); //Mark high 64 bits.
					*reinterpret_cast<uint64_t *>(reinterpret_cast<uint8_t *>(&BinaryAddr) + sizeof(in6_addr) / 2U) = 0; //Delete low 64 bits.
				}
				else {
					*reinterpret_cast<uint64_t *>(reinterpret_cast<uint8_t *>(&BinaryAddr) + sizeof(in6_addr) / 2U) = hton64(ntoh64(*reinterpret_cast<uint64_t *>(reinterpret_cast<uint8_t *>(&BinaryAddr) + sizeof(in6_addr) / 2U)) & (UINT64_MAX << (sizeof(in6_addr) * BYTES_TO_BITS - EDNS_Subnet_Header->Netmask_Source))); //Mark low 64 bits.
				}
			}
			else {
				EDNS_Subnet_Header->Netmask_Source = static_cast<uint8_t>(Parameter.LocalMachineSubnet_IPv6->second);
			}

		//Length check
			DataLength += sizeof(edns_client_subnet);
			if (DataLength + sizeof(in6_addr) >= MaxLen)
				return DataLength;

		//Copy subnet address.
			size_t PrefixBytes = 0;
			if (EDNS_Subnet_Header->Netmask_Source % BYTES_TO_BITS > 0)
				PrefixBytes = EDNS_Subnet_Header->Netmask_Source / BYTES_TO_BITS + sizeof(uint8_t);
			else 
				PrefixBytes = EDNS_Subnet_Header->Netmask_Source / BYTES_TO_BITS;
			if (Parameter.EDNS_ClientSubnet_Relay && LocalSocketData != nullptr && LocalSocketData->SockAddr.ss_family == AF_INET6)
				*reinterpret_cast<in6_addr *>(Buffer + DataLength) = BinaryAddr;
			else 
				*reinterpret_cast<in6_addr *>(Buffer + DataLength) = reinterpret_cast<sockaddr_in6 *>(&Parameter.LocalMachineSubnet_IPv6->first)->sin6_addr;
			EDNS_Subnet_Header->Length = htons(static_cast<uint16_t>(sizeof(uint16_t) + sizeof(uint8_t) * 2U + PrefixBytes));
			DNS_Record_OPT->DataLength = htons(static_cast<uint16_t>(sizeof(edns_client_subnet) + PrefixBytes));
			DataLength += PrefixBytes;
		}
	//A record(IPv4)
		else if (ntohs(DNS_Query->Type) == DNS_TYPE_A && 
			((Parameter.EDNS_ClientSubnet_Relay && LocalSocketData != nullptr && LocalSocketData->SockAddr.ss_family == AF_INET) || 
			Parameter.LocalMachineSubnet_IPv4 != nullptr))
		{
		//Make EDNS Subnet header.
			EDNS_Subnet_Header->Code = htons(EDNS_CODE_CSUBNET);
			EDNS_Subnet_Header->Family = htons(EDNS_ADDRESS_FAMILY_IPV4);

		//Mark network prefix.
			in_addr BinaryAddr;
			memset(&BinaryAddr, 0, sizeof(BinaryAddr));
			if (Parameter.EDNS_ClientSubnet_Relay && LocalSocketData != nullptr && LocalSocketData->SockAddr.ss_family == AF_INET)
			{
			//Default prefix of IPv4 address, please visit RFC 7871(https://tools.ietf.org/html/rfc7871).
				if (Parameter.LocalMachineSubnet_IPv4 != nullptr)
					EDNS_Subnet_Header->Netmask_Source = static_cast<uint8_t>(Parameter.LocalMachineSubnet_IPv4->second);
				else 
					EDNS_Subnet_Header->Netmask_Source = EDNS_CLIENT_SUBNET_SOURCE_PREFIX_IPV4;

			//Keep bits of address.
				BinaryAddr = reinterpret_cast<const sockaddr_in *>(&LocalSocketData->SockAddr)->sin_addr;
				BinaryAddr.s_addr = htonl(ntohl(BinaryAddr.s_addr) & (UINT32_MAX << (sizeof(in_addr) * BYTES_TO_BITS - EDNS_Subnet_Header->Netmask_Source)));
			}
			else {
				EDNS_Subnet_Header->Netmask_Source = static_cast<uint8_t>(Parameter.LocalMachineSubnet_IPv4->second);
			}

		//Length check
			DataLength += sizeof(edns_client_subnet);
			if (DataLength + sizeof(in_addr) >= MaxLen)
				return DataLength;

		//Copy subnet address.
			size_t PrefixBytes = 0;
			if (EDNS_Subnet_Header->Netmask_Source % BYTES_TO_BITS > 0)
				PrefixBytes = EDNS_Subnet_Header->Netmask_Source / BYTES_TO_BITS + sizeof(uint8_t);
			else 
				PrefixBytes = EDNS_Subnet_Header->Netmask_Source / BYTES_TO_BITS;
			if (Parameter.EDNS_ClientSubnet_Relay && LocalSocketData != nullptr && LocalSocketData->SockAddr.ss_family == AF_INET)
				*reinterpret_cast<in_addr *>(Buffer + DataLength) = BinaryAddr;
			else 
				*reinterpret_cast<in_addr *>(Buffer + DataLength) = reinterpret_cast<sockaddr_in *>(&Parameter.LocalMachineSubnet_IPv4->first)->sin_addr;
			EDNS_Subnet_Header->Length = htons(static_cast<uint16_t>(sizeof(uint16_t) + sizeof(uint8_t) * 2U + PrefixBytes));
			DNS_Record_OPT->DataLength = htons(static_cast<uint16_t>(sizeof(edns_client_subnet) + PrefixBytes));
			DataLength += PrefixBytes;
		}
	}

	return DataLength;
}

//Add EDNS label to DNS packet(DNS packet structure)
bool Add_EDNS_LabelToPacket(
	DNS_PACKET_DATA * const PacketStructure, 
	const SOCKET_DATA * const LocalSocketData)
{
//Initialization
	const auto DNS_Header = reinterpret_cast<dns_hdr *>(PacketStructure->Buffer);
	edns_header *EDNS_Header = nullptr;
	auto IsEDNS_ClientSubnet = false;

//EDNS Client Subnet check(Part 1)
	if ((Parameter.EDNS_ClientSubnet_Relay && LocalSocketData != nullptr) || Parameter.LocalMachineSubnet_IPv6 != nullptr || Parameter.LocalMachineSubnet_IPv4 != nullptr)
		IsEDNS_ClientSubnet = true;

//Packet with EDNS Label
	if (PacketStructure->EDNS_Location > 0)
	{
	//EDNS Client Subnet check(Part 2)
		if (IsEDNS_ClientSubnet)
		{
		//Match EDNS Client Subnet record.
			for (size_t Index = PacketStructure->EDNS_Location + sizeof(edns_header);Index < PacketStructure->EDNS_Location + PacketStructure->EDNS_Length;)
			{
				const auto EDNS_DataOption = reinterpret_cast<edns_data_option *>(PacketStructure->Buffer + Index);
				if (ntohs(EDNS_DataOption->Code) == EDNS_CODE_CSUBNET)
				{
					IsEDNS_ClientSubnet = false;
					break;
				}

			//Next loop
				Index += sizeof(edns_data_option) + ntohs(EDNS_DataOption->Length);
			}

		//EDNS Label length check.
			if (IsEDNS_ClientSubnet && PacketStructure->Length + sizeof(edns_data_option) * 2U + sizeof(in6_addr) + sizeof(in_addr) >= PacketStructure->BufferSize)
				return false;
		}

	//Update EDNS Label information.
		EDNS_Header = reinterpret_cast<edns_header *>(PacketStructure->Buffer + PacketStructure->EDNS_Location);
	}
//Packet without EDNS Label
	else {
	//EDNS Label length check
		if ((IsEDNS_ClientSubnet && PacketStructure->Length + sizeof(edns_header) + sizeof(edns_data_option) * 2U + sizeof(in6_addr) + sizeof(in_addr) >= PacketStructure->BufferSize) || 
			(!IsEDNS_ClientSubnet && PacketStructure->Length + sizeof(edns_header) >= PacketStructure->BufferSize))
				return false;

	//Make a new EDNS/OPT Additional Resource Record.
		memset(PacketStructure->Buffer + PacketStructure->Length, 0, PacketStructure->BufferSize - PacketStructure->Length);
		EDNS_Header = reinterpret_cast<edns_header *>(PacketStructure->Buffer + PacketStructure->Length);
		EDNS_Header->Type = htons(DNS_TYPE_OPT);
		EDNS_Header->UDP_PayloadSize = htons(static_cast<uint16_t>(Parameter.EDNS_PayloadSize));

	//Update EDNS Label information.
		PacketStructure->EDNS_Location = PacketStructure->Length;
		PacketStructure->EDNS_Length = sizeof(edns_header);
		PacketStructure->Records_Location.push_back(PacketStructure->Length);
		PacketStructure->Records_Length.push_back(sizeof(edns_header));
		PacketStructure->Length += sizeof(edns_header);

	//Rebuild DNS header counts.
		++PacketStructure->Records_AdditionalCount;
		DNS_Header->Additional = htons(static_cast<uint16_t>(PacketStructure->Records_AdditionalCount));
	}

//DNSSEC request
	if (Parameter.DNSSEC_Request)
	{
		DNS_Header->Flags = htons(ntohs(DNS_Header->Flags) | DNS_FLAG_GET_BIT_AD); //Set Authentic Data bit.
//		DNS_Header->Flags = htons(ntohs(DNS_Header->Flags) | DNS_FLAG_GET_BIT_CD); //Set Checking Disabled bit.
		EDNS_Header->Z_Field = htons(ntohs(EDNS_Header->Z_Field) | EDNS_FLAG_GET_BIT_DO); //Set Accepts DNSSEC security Resource Records bit.
	}

//EDNS Client Subnet
	if (IsEDNS_ClientSubnet)
	{
	//DNS Classes and Type check
		const auto DNS_Query = reinterpret_cast<dns_qry *>(PacketStructure->Buffer + DNS_PACKET_QUERY_LOCATE(PacketStructure->Buffer));
		if (ntohs(DNS_Query->Classes) != DNS_CLASS_INTERNET || (ntohs(DNS_Query->Type) != DNS_TYPE_AAAA && ntohs(DNS_Query->Type) != DNS_TYPE_A))
			return true;

	//EDNS Client Subnet initialization
		const auto EDNS_ClientSubnetHeader = reinterpret_cast<edns_client_subnet *>(PacketStructure->Buffer + PacketStructure->EDNS_Location + PacketStructure->EDNS_Length);
		auto IsLocalRelay = false, IsGlobalRelay = false;

	//AAAA record(IPv6)
		if (ntohs(DNS_Query->Type) == DNS_TYPE_AAAA)
		{
			if (Parameter.EDNS_ClientSubnet_Relay && LocalSocketData != nullptr && LocalSocketData->SockAddr.ss_family == AF_INET6)
				IsLocalRelay = true;
			else if (Parameter.LocalMachineSubnet_IPv6 != nullptr)
				IsGlobalRelay = true;
			if (IsLocalRelay || IsGlobalRelay)
			{
			//Make EDNS Subnet header.
				EDNS_ClientSubnetHeader->Code = htons(EDNS_CODE_CSUBNET);
				EDNS_ClientSubnetHeader->Family = htons(EDNS_ADDRESS_FAMILY_IPV6);

			//Mark network prefix address.
				in6_addr BinaryAddr;
				memset(&BinaryAddr, 0, sizeof(BinaryAddr));
				if (IsLocalRelay)
				{
				//Default prefix of IPv6 address, please visit RFC 7871(https://tools.ietf.org/html/rfc7871).
					if (Parameter.LocalMachineSubnet_IPv6 != nullptr)
						EDNS_ClientSubnetHeader->Netmask_Source = static_cast<uint8_t>(Parameter.LocalMachineSubnet_IPv6->second);
					else 
						EDNS_ClientSubnetHeader->Netmask_Source = EDNS_CLIENT_SUBNET_SOURCE_PREFIX_IPV6;

				//Keep bits of address.
					BinaryAddr = reinterpret_cast<const sockaddr_in6 *>(&LocalSocketData->SockAddr)->sin6_addr;
					if (EDNS_ClientSubnetHeader->Netmask_Source < sizeof(in6_addr) * BYTES_TO_BITS / 2U)
					{
						*reinterpret_cast<uint64_t *>(&BinaryAddr) = hton64(ntoh64(*reinterpret_cast<uint64_t *>(&BinaryAddr)) & (UINT64_MAX << (sizeof(in6_addr) * BYTES_TO_BITS / 2U - EDNS_ClientSubnetHeader->Netmask_Source))); //Mark high 64 bits.
						*reinterpret_cast<uint64_t *>(reinterpret_cast<uint8_t *>(&BinaryAddr) + sizeof(in6_addr) / 2U) = 0; //Delete low 64 bits.
					}
					else {
						*reinterpret_cast<uint64_t *>(reinterpret_cast<uint8_t *>(&BinaryAddr) + sizeof(in6_addr) / 2U) = hton64(ntoh64(*reinterpret_cast<uint64_t *>(reinterpret_cast<uint8_t *>(&BinaryAddr) + sizeof(in6_addr) / 2U)) & (UINT64_MAX << (sizeof(in6_addr) * BYTES_TO_BITS - EDNS_ClientSubnetHeader->Netmask_Source))); //Mark low 64 bits.
					}
				}
				else {
					EDNS_ClientSubnetHeader->Netmask_Source = static_cast<uint8_t>(Parameter.LocalMachineSubnet_IPv6->second);
				}

			//Copy client subnet address.
				size_t PrefixBytes = 0;
				if (EDNS_ClientSubnetHeader->Netmask_Source % BYTES_TO_BITS > 0)
					PrefixBytes = EDNS_ClientSubnetHeader->Netmask_Source / BYTES_TO_BITS + sizeof(uint8_t);
				else 
					PrefixBytes = EDNS_ClientSubnetHeader->Netmask_Source / BYTES_TO_BITS;
				if (IsLocalRelay)
					*reinterpret_cast<in6_addr *>(PacketStructure->Buffer + PacketStructure->EDNS_Location + PacketStructure->EDNS_Length + sizeof(edns_client_subnet)) = BinaryAddr;
				else 
					*reinterpret_cast<in6_addr *>(PacketStructure->Buffer + PacketStructure->EDNS_Location + PacketStructure->EDNS_Length + sizeof(edns_client_subnet)) = reinterpret_cast<sockaddr_in6 *>(&Parameter.LocalMachineSubnet_IPv6->first)->sin6_addr;

			//Update EDNS Client Subnet information.
				EDNS_ClientSubnetHeader->Length = htons(ntohs(EDNS_ClientSubnetHeader->Length) + static_cast<uint16_t>(sizeof(EDNS_ClientSubnetHeader->Family) + sizeof(EDNS_ClientSubnetHeader->Netmask_Source) + sizeof(EDNS_ClientSubnetHeader->Netmask_Scope) + PrefixBytes));
				EDNS_Header->DataLength = htons(static_cast<uint16_t>(ntohs(EDNS_Header->DataLength) + sizeof(edns_client_subnet) + PrefixBytes));
				PacketStructure->Length += sizeof(edns_client_subnet) + PrefixBytes;
				PacketStructure->Records_Length.back() += sizeof(edns_client_subnet) + PrefixBytes;
				PacketStructure->EDNS_Length += sizeof(edns_client_subnet) + PrefixBytes;
			}
		}
	//A record(IPv4)
		else if (ntohs(DNS_Query->Type) == DNS_TYPE_A)
		{
			if (Parameter.EDNS_ClientSubnet_Relay && LocalSocketData != nullptr && LocalSocketData->SockAddr.ss_family == AF_INET)
				IsLocalRelay = true;
			else if (Parameter.LocalMachineSubnet_IPv4 != nullptr)
				IsGlobalRelay = true;
			if (IsLocalRelay || IsGlobalRelay)
			{
			//Make EDNS Subnet header.
				EDNS_ClientSubnetHeader->Code = htons(EDNS_CODE_CSUBNET);
				EDNS_ClientSubnetHeader->Family = htons(EDNS_ADDRESS_FAMILY_IPV4);

			//Mark network prefix address.
				in_addr BinaryAddr;
				memset(&BinaryAddr, 0, sizeof(BinaryAddr));
				if (IsLocalRelay)
				{
				//Default prefix of IPv4 address, please visit RFC 7871(https://tools.ietf.org/html/rfc7871).
					if (Parameter.LocalMachineSubnet_IPv4 != nullptr)
						EDNS_ClientSubnetHeader->Netmask_Source = static_cast<uint8_t>(Parameter.LocalMachineSubnet_IPv4->second);
					else 
						EDNS_ClientSubnetHeader->Netmask_Source = EDNS_CLIENT_SUBNET_SOURCE_PREFIX_IPV4;

				//Keep bits of address.
					BinaryAddr = reinterpret_cast<const sockaddr_in *>(&LocalSocketData->SockAddr)->sin_addr;
					BinaryAddr.s_addr = htonl(ntohl(BinaryAddr.s_addr) & (UINT32_MAX << (sizeof(in_addr) * BYTES_TO_BITS - EDNS_ClientSubnetHeader->Netmask_Source)));
				}
				else {
					EDNS_ClientSubnetHeader->Netmask_Source = static_cast<uint8_t>(Parameter.LocalMachineSubnet_IPv4->second);
				}

			//Copy client subnet address.
				size_t PrefixBytes = 0;
				if (EDNS_ClientSubnetHeader->Netmask_Source % BYTES_TO_BITS > 0)
					PrefixBytes = EDNS_ClientSubnetHeader->Netmask_Source / BYTES_TO_BITS + sizeof(uint8_t);
				else 
					PrefixBytes = EDNS_ClientSubnetHeader->Netmask_Source / BYTES_TO_BITS;
				if (IsLocalRelay)
					*reinterpret_cast<in_addr *>(PacketStructure->Buffer + PacketStructure->EDNS_Location + PacketStructure->EDNS_Length + sizeof(edns_client_subnet)) = BinaryAddr;
				else 
					*reinterpret_cast<in_addr *>(PacketStructure->Buffer + PacketStructure->EDNS_Location + PacketStructure->EDNS_Length + sizeof(edns_client_subnet)) = reinterpret_cast<sockaddr_in *>(&Parameter.LocalMachineSubnet_IPv4->first)->sin_addr;

			//Update EDNS Client Subnet information.
				EDNS_ClientSubnetHeader->Length = htons(ntohs(EDNS_ClientSubnetHeader->Length) + static_cast<uint16_t>(sizeof(EDNS_ClientSubnetHeader->Family) + sizeof(EDNS_ClientSubnetHeader->Netmask_Source) + sizeof(EDNS_ClientSubnetHeader->Netmask_Scope) + PrefixBytes));
				EDNS_Header->DataLength = htons(static_cast<uint16_t>(ntohs(EDNS_Header->DataLength) + sizeof(edns_client_subnet) + PrefixBytes));
				PacketStructure->Length += sizeof(edns_client_subnet) + PrefixBytes;
				PacketStructure->Records_Length.back() += sizeof(edns_client_subnet) + PrefixBytes;
				PacketStructure->EDNS_Length += sizeof(edns_client_subnet) + PrefixBytes;
			}
		}
	}

	return true;
}

//Make Compression Pointer Mutation
size_t MakeCompressionPointerMutation(
	uint8_t * const Buffer, 
	const size_t Length, 
	const size_t MaxLen)
{
//Ramdom number distribution initialization
	std::uniform_int_distribution<uint64_t> RamdomDistribution(0, 2U);
	auto Index = RamdomDistribution(*GlobalRunningStatus.RamdomEngine);

//Check Compression Pointer Mutation options.
	switch (Index)
	{
		case CPM_POINTER_TYPE_HEADER:
		{
			if (!Parameter.CPM_PointerToHeader)
			{
				if (Parameter.CPM_PointerToRR)
					Index = CPM_POINTER_TYPE_RR;
				else //Pointer to Additional(2)
					Index = CPM_POINTER_TYPE_ADDITIONAL;
			}
		}break;
		case CPM_POINTER_TYPE_RR:
		{
			if (!Parameter.CPM_PointerToRR)
			{
				if (Parameter.CPM_PointerToHeader)
					Index = CPM_POINTER_TYPE_HEADER;
				else //Pointer to Additional(2)
					Index = CPM_POINTER_TYPE_ADDITIONAL;
			}
		}break;
		case CPM_POINTER_TYPE_ADDITIONAL:
		{
			if (!Parameter.CPM_PointerToAdditional)
			{
				if (Parameter.CPM_PointerToHeader)
					Index = CPM_POINTER_TYPE_HEADER;
				else //Pointer to RR
					Index = CPM_POINTER_TYPE_RR;
			}
		}break;
		default:
		{
			return EXIT_FAILURE;
		}
	}

//Make Compression Pointer Mutation.
	if (Index == CPM_POINTER_TYPE_HEADER) //Pointer to header, like "<DNS Header><Domain><Pointer><Query>" and point to <DNS Header>.
	{
		memmove_s(Buffer + Length - sizeof(dns_qry) + NULL_TERMINATE_LENGTH, sizeof(dns_qry), Buffer + Length - sizeof(dns_qry), sizeof(dns_qry));
		*(Buffer + Length - sizeof(dns_qry) - 1U) = static_cast<uint8_t>(DNS_POINTER_8_BITS_STRING);

	//Choose a ramdom one.
		Index = GetCurrentSystemTime() % 4U;
		switch (Index)
		{
			case 0:
			{
				*(Buffer + Length - sizeof(dns_qry)) = ('\x04');
			}break;
			case 1U:
			{
				*(Buffer + Length - sizeof(dns_qry)) = ('\x06');
			}break;
			case 2U:
			{
				*(Buffer + Length - sizeof(dns_qry)) = ('\x08');
			}break;
			case 3U:
			{
				*(Buffer + Length - sizeof(dns_qry)) = ('\x0A');
			}break;
			default:
			{
				return EXIT_FAILURE;
			}
		}

		return Length + 1U;
	}
	else {
		dns_qry DNS_Query;
		memset(&DNS_Query, 0, sizeof(dns_qry));
		memcpy_s(&DNS_Query, sizeof(dns_qry), Buffer + DNS_PACKET_QUERY_LOCATE(Buffer), sizeof(DNS_Query));
		memmove_s(Buffer + sizeof(dns_hdr) + sizeof(uint16_t) + sizeof(dns_qry), MaxLen - sizeof(dns_hdr) - sizeof(uint16_t) - sizeof(dns_qry), Buffer + sizeof(dns_hdr), strnlen_s(reinterpret_cast<const char *>(Buffer) + sizeof(dns_hdr), Length - sizeof(dns_hdr)) + 1U);
		memcpy_s(Buffer + sizeof(dns_hdr) + sizeof(uint16_t), MaxLen - sizeof(dns_hdr) - sizeof(uint16_t), &DNS_Query, sizeof(DNS_Query));
		*(Buffer + sizeof(dns_hdr)) = static_cast<uint8_t>(DNS_POINTER_8_BITS_STRING);
		*(Buffer + sizeof(dns_hdr) + 1U) = ('\x12');

	//Pointer to RR, like "<DNS Header><Pointer><Query><Domain>" and point to <Domain>.
		if (Index == CPM_POINTER_TYPE_RR)
		{
			return Length + 2U;
		}
	//Pointer to Additional, like "<DNS Header><Pointer><Query><Additional>" and point domain to <Additional>.
		else {
		//Ramdom number distribution initialization
			std::uniform_int_distribution<uint32_t> RamdomDistribution_Additional(0, UINT32_MAX);

		//Make records.
			reinterpret_cast<dns_hdr *>(Buffer)->Additional = htons(UINT16_NUM_ONE);
			if (ntohs(DNS_Query.Type) == DNS_TYPE_AAAA)
			{
				const auto DNS_Record_AAAA = reinterpret_cast<dns_record_aaaa *>(Buffer + Length);
				DNS_Record_AAAA->Type = htons(DNS_TYPE_AAAA);
				DNS_Record_AAAA->Classes = htons(DNS_CLASS_INTERNET);
				DNS_Record_AAAA->TTL = htonl(RamdomDistribution_Additional(*GlobalRunningStatus.RamdomEngine));
				DNS_Record_AAAA->Length = htons(sizeof(in6_addr));
				for (Index = 0;Index < sizeof(in6_addr) / sizeof(uint8_t);++Index)
					DNS_Record_AAAA->Address.s6_addr[Index] = static_cast<uint8_t>(RamdomDistribution_Additional(*GlobalRunningStatus.RamdomEngine));

				return Length + sizeof(dns_record_aaaa);
			}
//			else if (ntohs(DNS_Query.Type) == DNS_TYPE_A)
			else { //A record
				const auto DNS_Record_A = reinterpret_cast<dns_record_a *>(Buffer + Length);
				DNS_Record_A->Type = htons(DNS_TYPE_A);
				DNS_Record_A->Classes = htons(DNS_CLASS_INTERNET);
				DNS_Record_A->TTL = htonl(RamdomDistribution_Additional(*GlobalRunningStatus.RamdomEngine));
				DNS_Record_A->Length = htons(sizeof(in_addr));
				DNS_Record_A->Address.s_addr = htonl(RamdomDistribution_Additional(*GlobalRunningStatus.RamdomEngine));

				return Length + sizeof(dns_record_a);
			}
		}
	}

	return EXIT_FAILURE;
}
