#ifndef AUDIO
#define AUDIO

#include <utils/misc.h>
#include <string.h>

struct sound
{
    int16* buffer;
    uint buffer_length;
};

struct noise
{
    sound* snd;
    real volume;
    uint samples_played;
    uint64 current_sample;
};

struct audio_data
{
    int16* buffer;
    uint buffer_length;
    uint sample_rate;
    IXAudio2SourceVoice* source_voice = 0;
    uint64 samples_played;

    noise* noises;
    int n_noises;
};

sound waterfall;
sound jump_sound;

void update_sounds(audio_data* ad)
{
    XAUDIO2_VOICE_STATE voice_state = {};
    ad->source_voice->GetState(&voice_state, 0);
    uint64 new_samples_played = voice_state.SamplesPlayed;

    for(uint i = ad->samples_played; i < new_samples_played; i++)
    {
        ad->buffer[i%ad->buffer_length] = 0;
    }
    ad->samples_played = new_samples_played;

    for(int n = ad->n_noises-1; n >= 0; n--)
    {
        noise* ns = &ad->noises[n];
        if(ns->samples_played == 0) ns->current_sample = ad->samples_played;
        ns->current_sample = max(ns->current_sample, ad->samples_played);
        while(ns->current_sample < ad->samples_played+ad->buffer_length)
        {
            uint buffer_index = (ns->current_sample++)%ad->buffer_length;
            ad->buffer[buffer_index] += ns->volume*ns->snd->buffer[ns->samples_played++];
            if(ns->samples_played >= ns->snd->buffer_length)
            {
                ad->noises[n] = ad->noises[--ad->n_noises];
                break;
            }
        }
    }
}

struct riff_chunk
{
    char tag[4];
    uint32 size;
    byte* data;
};

riff_chunk read_riff_chunk(memory_manager* manager, file_t file, size_t offset)
{
    riff_chunk out;
    read_from_disk(out.tag, file, offset, sizeof(out.tag));
    offset += sizeof(out.tag);
    read_from_disk(&out.size, file, offset, sizeof(out.size));
    offset += sizeof(out.size);
    out.data = permalloc(manager, out.size);
    read_from_disk(out.data, file, offset, out.size);
    return out;
}


#pragma pack(push, 1)
struct wav_format
{
    uint16 wFormatTag;
    uint16 nChannels;
    uint32 nSamplesPerSec;
    uint32 nAvgBytesPerSec;
    uint16 nBlockAlign;
    uint16 wBitsPerSample;
    uint16 cbSize;
    uint16 wValidBitsPerSample;
    uint32 dwChannelMask;
    byte SubFormat[16];
};
#pragma pack(pop)

sound load_wav(memory_manager* manager, char* filename)
{
    file_t file = open_file(filename, OPEN_EXISTING);
    size_t file_size = sizeof_file(file);

    size_t offset = 0;
    riff_chunk riff = read_riff_chunk(manager, file, offset);
    assert(strncmp((char*) riff.tag, "RIFF", 4) == 0);
    assert(strncmp((char*) riff.data, "WAVE", 4) == 0);
    offset += 12;

    riff_chunk fmt_chunk = read_riff_chunk(manager, file, offset);
    offset += 12+((fmt_chunk.size+1)&~1);
    wav_format* format = (wav_format*) fmt_chunk.data;

    {
        log_output("wFormatTag          : ", format->wFormatTag, "\n");
        log_output("nChannels           : ", format->nChannels, "\n");
        log_output("nSamplesPerSec      : ", format->nSamplesPerSec, "\n");
        log_output("nAvgBytesPerSec     : ", format->nAvgBytesPerSec, "\n");
        log_output("nBlockAlign         : ", format->nBlockAlign, "\n");
        log_output("wBitsPerSample      : ", format->wBitsPerSample, "\n");
        log_output("cbSize              : ", format->cbSize, "\n");
        log_output("wValidBitsPerSample : ", format->wValidBitsPerSample, "\n");
        log_output("dwChannelMask       : ", format->dwChannelMask, "\n");
        log_output("\n");
    }

    size_t buffer_size = file_size-offset;
    sound out = {
        .buffer = (int16*) permalloc(manager, buffer_size),
        .buffer_length = buffer_size/sizeof(int16),
    };
    read_from_disk(out.buffer, file, offset, buffer_size);

    if(format->nChannels > 1)
    {
        out.buffer_length = out.buffer_length/format->nChannels;
        for(int i = 0; i < out.buffer_length; i++)
        {
            uint bit = i*format->wBitsPerSample;
            out.buffer[i] = (*((uint32*)((byte*) out.buffer+bit/8))>>(bit%8)) & ((1<<(format->wBitsPerSample))-1);
        }
    }
    close_file(file);
    return out;
}

sound load_audio_file(memory_manager* manager, char* filename)
{
    //there should be more formats here later, selected based on file extension
    return load_wav(manager, filename);
}

void play_sound(audio_data* ad, sound* snd, real volume)
{
    ad->noises[ad->n_noises++] = {
        .snd = snd,
        .volume = volume,
        .samples_played = 0,
    };
}

#endif //AUDIO
