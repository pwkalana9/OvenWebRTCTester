// csv_converter.cpp : 이 파일에는 'main' 함수가 포함됩니다. 거기서 프로그램 실행이 시작되고 종료됩니다.
//
#include <iostream>
#include <fstream>
#include <vector>
#include <sstream>
#include <memory>

#define CSV_LINE_SEPARATOR  (',')
#define CSV_LINE_END		('\n')

struct CollectResultData
{
	uint32_t line_index = 0; 
	uint32_t collect_time = 0;
	uint32_t stream_count = 0;
	uint32_t network_bitrate = 0; // kbps
	uint32_t video_framerate = 0; // fps 
	uint32_t audio_framerate = 0; // fps
	uint32_t video_latency_fail = 0;	// +20ms ~ -20ms  
	uint32_t audio_latency_fail = 0;	// +20ms ~ -20ms  
	uint32_t video_latency = 0;	// ms (절대값 총합)  
	uint32_t audio_latency = 0;	// ms (절대값 총합) 
	uint32_t packet_loss_percent = 0;
	uint32_t packet_loss_count = 0;
};

//====================================================================================================
// split
//====================================================================================================
std::vector<std::string> split(const std::string &text, const char delim)
{
	std::vector<std::string> result; 
	std::stringstream sstream(text); 
	std::string item; 

	while (std::getline(sstream, item, delim))
	{
		result.push_back(item); 
	}

	return result; 
}

//====================================================================================================
// main
//====================================================================================================
int main(int argc, char* argv[])
{
   	int file_count = argc - 1;

	std::vector<std::shared_ptr<std::ifstream>> input_files;
	std::ofstream result_file("result.csv");
	// file open 
	for (int index = 0; index < file_count; index++)
	{
		auto file = std::make_shared<std::ifstream>(argv[index + 1]);
		if (file->fail())
		{
			printf("file open fail");
		}

		input_files.push_back(file);
	}
	

	// header write 
	result_file << "time(sec),"
				<< "stream(count),"
				<< "network bitrate(kbps),"
				<< "video framerate(fps avg),"
				<< "audio framerate(fps avg),"
				<< "video latency fail(count),"
				<< "audio latency fail(count),"
				<< "video latency(ms avg),"
				<< "audio latency(ms avg),"
				<< "packet loss(percent)"
				<< "packet loss(count)"
				<< CSV_LINE_END;
	
	//read data 
	std::vector<std::shared_ptr<CollectResultData>> collect_datas;
	int max_index = -1; 
	for (const auto &input_file : input_files)
	{
		int line_index = 0; 
		std::string line;

		// header skip 
		std::getline(*input_file, line);
		std::getline(*input_file, line);

		while (!input_file->eof())
		{
			

			std::getline(*input_file, line);

			if (line.empty())
				break;
			
			std::vector<std::string> split_result = split(line, CSV_LINE_SEPARATOR);

			std::shared_ptr<CollectResultData> collect_data = nullptr;


			if (max_index < line_index)
			{
				collect_data = std::make_shared<CollectResultData>();
				collect_datas.push_back(collect_data);

				max_index = line_index; 
			}
			else
				collect_data = collect_datas[line_index];

			collect_data->collect_time			+= std::stoi(split_result[0]);
			collect_data->stream_count			+= std::stoi(split_result[1]);
			collect_data->network_bitrate		+= std::stoi(split_result[2]);
			collect_data->video_framerate		+= std::stoi(split_result[3]);
			collect_data->audio_framerate		+= std::stoi(split_result[4]);
			collect_data->video_latency_fail	+= std::stoi(split_result[5]);
			collect_data->audio_latency_fail	+= std::stoi(split_result[6]);
			collect_data->video_latency			+= std::stoi(split_result[7]);
			collect_data->audio_latency			+= std::stoi(split_result[8]);
			collect_data->packet_loss_percent	+= std::stoi(split_result[9]);
			collect_data->packet_loss_count		+= std::stoi(split_result[10]);

			line_index++;
		}
	}

	// write data 
	for (const auto &collect_data : collect_datas)
	{
		// CSV 파일 저정
		result_file
			<< collect_data->collect_time / file_count << CSV_LINE_SEPARATOR
			<< collect_data->stream_count  << CSV_LINE_SEPARATOR
			<< collect_data->network_bitrate << CSV_LINE_SEPARATOR
			<< collect_data->video_framerate / file_count << CSV_LINE_SEPARATOR
			<< collect_data->audio_framerate / file_count << CSV_LINE_SEPARATOR
			<< collect_data->video_latency_fail << CSV_LINE_SEPARATOR
			<< collect_data->audio_latency_fail << CSV_LINE_SEPARATOR
			<< collect_data->video_latency / file_count << CSV_LINE_SEPARATOR
			<< collect_data->audio_latency / file_count << CSV_LINE_SEPARATOR
			<< collect_data->packet_loss_percent / file_count << CSV_LINE_SEPARATOR
			<< collect_data->packet_loss_count << CSV_LINE_END;
	}

		
	// file close 
	for (const auto &file : input_files)
	{
		file->close(); 
	}
	result_file.close(); 
}

