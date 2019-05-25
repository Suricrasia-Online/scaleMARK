#include <opus/opus.h>
#include <cstdint>
#include <iostream>
#include <fstream>
#include <vector>
#include <iterator>
#include <unistd.h>

static const int channels = 2;
static const int frame_size_120ms = 5760;

int check_opus_error(int error) {
	if (error < OPUS_OK) {
		std::cerr << "Opus Error: " << error << std::endl;
		return -1;
	}
	return 0;
}

std::vector<char> read_samples(const std::string& filename) {
	std::ifstream sampleReader(filename, std::ios::binary);

	std::istreambuf_iterator<char> start(sampleReader);
	std::istreambuf_iterator<char> end;

	return std::vector<char>(start, end);
}

std::vector<unsigned char> encode_packet(const std::vector<char>& pcm, int sizelimit, int rounds, const std::vector<int>& encoder_ctl, int* error) {
	std::vector<unsigned char> packet(sizelimit);

	//create encoder
	auto encoder = opus_encoder_create(48000, channels, OPUS_APPLICATION_VOIP, error);
	if (*error < OPUS_OK) return packet;

	//set encoder settings
	for(auto ctl = encoder_ctl.begin(); ctl != encoder_ctl.end(); ctl++) {
		opus_encoder_ctl(encoder, *ctl);
	}

	//cast data to opus_int16
	auto pcm_raw_frame_size = pcm.size() / sizeof(opus_int16) / channels;
	auto pcm_raw = reinterpret_cast<const opus_int16*>(pcm.data());

	if (pcm_raw_frame_size < frame_size_120ms) {
		std::cerr << "PCM data doesn't contain enough samples" << std::endl;
		*error = OPUS_BAD_ARG;
		return packet;
	}

	//encode multiple times (or just once) to fool the encoder into using prediction
	int length = 0;
	for (int i = 0; i < rounds; i++) {
		length = opus_encode(encoder, pcm_raw, frame_size_120ms, packet.data(), packet.capacity());
		if (length < 0) {
			*error = length;
			return packet;
		}
	}
	packet.resize(length);

	return packet;
}

int main(int argc, char** argv) {
	int error;

	//load samples from file
	auto VY00RGLYL = read_samples("./VY00RGYL.raw");
	auto JOUVERT = read_samples("./JOUVERT.raw");
	auto SILENCE = read_samples("./silence.raw");
	auto RINGTONE = read_samples("./ringtone.raw");
	auto UHH = read_samples("./uhh.raw");

	auto voicectl = std::vector<int>({
		OPUS_SET_BITRATE(4000),
		OPUS_SET_COMPLEXITY(10),
		OPUS_SET_BANDWIDTH(OPUS_BANDWIDTH_MEDIUMBAND),
		OPUS_SET_SIGNAL(OPUS_SIGNAL_VOICE),
		OPUS_SET_APPLICATION(OPUS_APPLICATION_VOIP)
	});

	auto musicctl = std::vector<int>({
		OPUS_SET_BITRATE(4000),
		OPUS_SET_COMPLEXITY(5),
		OPUS_SET_VBR(0),
		OPUS_SET_BANDWIDTH(OPUS_BANDWIDTH_FULLBAND),
		OPUS_SET_SIGNAL(OPUS_SIGNAL_MUSIC),
		OPUS_SET_APPLICATION(OPUS_APPLICATION_AUDIO)
	});

	//encode samples
	auto VY00RGLYL_packet = encode_packet(VY00RGLYL, 150, 1, voicectl, &error);
	if (check_opus_error(error)) return -1;

	auto JOUVERT_packet = encode_packet(JOUVERT, 120, 4, voicectl, &error);
	if (check_opus_error(error)) return -1;

	auto SILENCE_packet = encode_packet(SILENCE, 20, 2, voicectl, &error);
	if (check_opus_error(error)) return -1;

	auto RINGTONE_packet = encode_packet(RINGTONE, 120, 6, musicctl, &error);
	if (check_opus_error(error)) return -1;

	auto UHH_packet = encode_packet(UHH, 150, 2, voicectl, &error);
	if (check_opus_error(error)) return -1;


	//decode the packet back to audio
	auto decoder = opus_decoder_create(48000, 2, &error);
	// opus_decoder_ctl(decoder, OPUS_SET_GAIN(-2000));
	std::cerr << VY00RGLYL_packet.size() << std::endl;
	std::cerr << JOUVERT_packet.size() << std::endl;
	std::cerr << SILENCE_packet.size() << std::endl;
	std::cerr << RINGTONE_packet.size() << std::endl;
	std::cerr << UHH_packet.size() << std::endl;

	// VY00RGLYL_packet[120]--;

	//generate 16 samples using the encoded data
	for (int i = 0; i < 164; i++) {

		std::vector<unsigned char> packet = SILENCE_packet;
		if (i%4 == 0 && i != 48) packet = VY00RGLYL_packet;
		if (i%4 == 2 && i > 32) packet = JOUVERT_packet;
		if (i > 40) {
			if (i%4 == 3 && (i/4)%4 == 0 && (i/16)%4 < 2) packet = JOUVERT_packet;
			if (i%4 == 1 && (i/4)%4 == 0 && (i/16)%4 >= 2) packet = JOUVERT_packet;
		}
		if (i == 89) packet = JOUVERT_packet;
		if (i >= 90-2 && i < 96) packet = SILENCE_packet;
		if (i == 90 || i == 91 || i == 92) packet = RINGTONE_packet;
		if (i == 95) packet = UHH_packet;
		if (i == 97) packet = JOUVERT_packet;
		std::vector<opus_int16> decoded_payload(frame_size_120ms*channels);
		int length = opus_decode(decoder, packet.data(), packet.size(), decoded_payload.data(), frame_size_120ms, 0);
		if (check_opus_error(length)) return -1;
		decoded_payload.resize(length*2);
		write(1, reinterpret_cast<const char*>(decoded_payload.data()), decoded_payload.size() * sizeof(opus_int16));
	}

	return 0;
}