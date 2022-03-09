#include <CoreMIDI/CoreMIDI.h>
#include "c74_min.h"

using namespace c74::min;

class min_midiout : public object<min_midiout> {
public:
    MIN_DESCRIPTION	{"MIDI out port as a device."};
    MIN_TAGS		{"utilities"};
    MIN_AUTHOR		{"Sam Nobs"};
    MIN_RELATED		{"midiin, midiout"};

    inlet<>  input	{ this, "(bytes) Raw MIDI from midiin" };

    min_midiout(const atoms& args = {}) {
        if (args.size() == 0)
            error("Need a MIDI port name");
        if (args.size() > 0)
            port_name = args[0];
        cout << "min.midiout: Port is " << port_name.get() << endl;
        OSStatus result = noErr;
        result = MIDIClientCreate(CFSTR("Max min.midiout"), &min_midiout::notifyMidi, this, &m_midi_client);
        if (result != noErr) {
            error("MIDIClientCreate() error: " + std::to_string(result));
        }

        result =  MIDIOutputPortCreate(m_midi_client, CFSTR("Max min.midiout out"), &m_port);
        if (result != noErr) {
            MIDIClientDispose(m_midi_client);
            error("MIDIOutputPortCreate() error: " + std::to_string(result));
        }

        m_output_port = 0;
        m_midi_len = 0;
        m_midi_ix = 0;
        refreshPorts();
    }

    ~min_midiout() {
        MIDIClientDispose(m_midi_client);
    }

    // the actual attribute for the message
    attribute<symbol> port_name { this, "port", "<not set>",
        description {
            "Name of the output MIDI port, as seen in Live's 'MIDI To' drop down menu. "
        }
    };


    // respond to the bang message to do something
    message<threadsafe::yes> bytes { this, "int", "Raw MIDI bytes.",
        MIN_FUNCTION {
            for (auto v: args)
            {
                int b = v;

                if (b & 0x80 and m_midi_len > 0) 
                {
                    cout << "flushing possibly incomplete message" << endl;
                    sendMidi();
                }

                if (m_midi_len == 0)
                {
                    // currently we only process the messages we know
                    //  the size of based on status byte
                    // TODO fall back to accumulating bytes anyway and sending
                    //      them out as soon as the next status byte is seen
                    int l = lengthForStatusByte(b);
                    if (l > 0)
                    {
                        m_midi_len = l;
                        m_midi_ix = 0;
                    }
                    else
                    {
                        cout << ": skipping " << b << endl;
                        continue;
                    }
                }

                m_midi_buf[m_midi_ix++] = b;
                if (m_midi_ix >= m_midi_len)
                {
                    sendMidi();
                }
            }
            return {};
        }
    };

    enum MidiType {
        NOTE_OFF         = 0x8,
        NOTE_ON          = 0x9,
        AFTERTOUCH       = 0xA,
        CONTROL_CHANGE   = 0xB,
        PROGRAM_CHANGE   = 0xC,
        CHANNEL_PRESSURE = 0xD,
        PITCH_WHEEL      = 0xE,
        COMMON_AND_RT    = 0xF
    };

    enum MidiSubType {
        SONG_POSITION_POINTER = 0x2,
        TIMING_CLOCK          = 0x8,
        START                 = 0xA,
        CONTINUE              = 0xB,
        STOP                  = 0xC
    };

    int lengthForStatusByte(uint8_t first_byte)
    {
        switch (first_byte >> 4)
        {
            case NOTE_OFF:
            case NOTE_ON:
            case AFTERTOUCH:
            case CONTROL_CHANGE:
            case PITCH_WHEEL:
                return 3;
            case PROGRAM_CHANGE:
            case CHANNEL_PRESSURE:
                return 2;
            case COMMON_AND_RT:
                switch (first_byte & 0xF)
                {
                    case SONG_POSITION_POINTER:
                    return 3;
                    case TIMING_CLOCK:
                    case START:
                    case CONTINUE:
                    case STOP:
                    return 1;
                    default:
                    // discard remainder, SYSEX etc not handled
                    return -1;
                }
            default:
                // discard remainder, SYSEX etc not handled
                return -1;
        }
    }

    void sendMidi()
    {
        if (m_output_port)
        {
            uint8_t buf[64];
            MIDIPacketList* packet_list = reinterpret_cast<MIDIPacketList*>(buf);
            MIDIPacket     *pkt = MIDIPacketListInit(packet_list);
            MIDIPacketListAdd(packet_list, sizeof(buf), pkt, 0, m_midi_len, m_midi_buf);
            MIDISend(m_port, m_output_port, packet_list);
        }
        m_midi_len = 0;
    }

    void refreshPorts()
    {

        int num_sources = MIDIGetNumberOfDestinations();
        
        // add new devices, if any
        for (int i = 0; i < num_sources; i++)
        {
            MIDIEndpointRef dev = MIDIGetDestination(i);

            std::stringstream description;
            CFStringRef name;
            char tmp[64];
            if (MIDIObjectGetStringProperty(dev, kMIDIPropertyModel, &name) == noErr)
            {
                CFStringGetCString(name, tmp, sizeof(tmp), 0);
                description << tmp << ": ";
            }
            if (MIDIObjectGetStringProperty(dev, kMIDIPropertyName, &name) == noErr)
            {
                CFStringGetCString(name, tmp, sizeof(tmp), 0);
                description << tmp;
            }
            if (description.str().size() == 0)
            {
                description << "Unknown device " << i;
            }

            cout << "Enumerating: " << description.str() << endl;

            if (description.str() == port_name.get())
            {
                cout << "Found what we were looking for!" << endl;
                if (m_output_port == 0)
                {
                    m_output_port = dev;
                }
                return;
            }

        }
      
      // port disappeared/not found
      m_output_port = 0;
    }
   
    static void notifyMidi(const MIDINotification *message, void *ref_con)
    {
        if (message->messageID == kMIDIMsgObjectAdded or
            message->messageID == kMIDIMsgObjectRemoved)
        {
            if (ref_con != nullptr) static_cast<min_midiout*>(ref_con)->refreshPorts();
        }
    }

private:

    MIDIClientRef   m_midi_client;
    MIDIEndpointRef m_output_port;
    MIDIPortRef     m_port;
    uint8_t         m_midi_buf[4];
    uint8_t         m_midi_ix;
    uint8_t         m_midi_len;
};


MIN_EXTERNAL(min_midiout);
