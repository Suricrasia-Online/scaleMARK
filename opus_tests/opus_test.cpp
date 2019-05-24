#include <opus/opus.h>
#include <cstdint>
#include <iostream>
#include <fstream>
#include <vector>
#include <iterator>

int check_opus_error(int error) {
	if (error < OPUS_OK) {
		std::cerr << "Opus Error: " << error << std::endl;
		return -1;
	}
	return 0;
}

std::vector<char> read_samples(std::string filename) {
	std::ifstream sampleReader(filename, std::ios::binary);

	std::istreambuf_iterator<char> start(sampleReader);
	std::istreambuf_iterator<char> end;

	return std::vector<char>(start, end);
}

int main(int argc, char** argv) {
	int error;

	//create encoder
	const int channels = 2;
	auto encoder = opus_encoder_create(48000, channels, OPUS_APPLICATION_AUDIO, &error);
	if (check_opus_error(error)) return -1;

	opus_encoder_ctl(encoder, OPUS_SET_BITRATE(4000));
	opus_encoder_ctl(encoder, OPUS_SET_COMPLEXITY(10));
	opus_encoder_ctl(encoder, OPUS_SET_BANDWIDTH(OPUS_BANDWIDTH_FULLBAND));
	opus_encoder_ctl(encoder, OPUS_SET_VBR(0));

	auto samples = read_samples("./sample.raw");

	auto samples_16_frame_size = samples.size() / sizeof(opus_int16) / channels;
	auto samples_16 = reinterpret_cast<const opus_int16*>(samples.data());

	std::vector<unsigned char> output_packet(500);

	const int frame_size_120ms = 5760;
	if (samples_16_frame_size < frame_size_120ms) {
		std::cerr << "sample doesn't contain enough samples" << std::endl;
	}

	// try encoding something
	int length = opus_encode(encoder, samples_16, frame_size_120ms, output_packet.data(), output_packet.capacity());
	if (check_opus_error(length)) return -1;
	output_packet.resize(length);

	std::cout << "Size of encoded packet: " << length << std::endl;

	return 0;
}