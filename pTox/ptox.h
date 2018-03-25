#ifndef PTOX_H
#define PTOX_H

#include <QObject>

#include <tox/tox.h>
#include <tox/toxav.h>

#include <iostream>
#include <sstream>
#include <string>
#include <chrono>
#include <thread>

#include <SFML/Audio.hpp>

#include "definitions.h"

class pTox : public QObject
{
    Q_OBJECT
public:
//    typedef enum STATUS {
//        AVAILABLE,
//        AWAY,
//        BUSY
//    } STATUS;
//    Q_ENUM(STATUS)
    struct toxFriend {
        std::string friendName;
        uint32_t friendNumber;
        uint8_t friendAddressBin[TOX_PUBLIC_KEY_SIZE];
        uint8_t friendAddressHex[TOX_PUBLIC_KEY_SIZE*2+1];
        std::string friendStatus;
        TOX_USER_STATUS friendConnectionStatus;
    };
private:
    Tox *tox;
    ToxAV *toxav;
    struct Tox_Options toxOptions;
    std::string FilePath;
    int framesize;

    pthread_t tox_thread, toxav_thread;

    uint8_t AddressBin[TOX_ADDRESS_SIZE];
    uint8_t AddressHex[TOX_ADDRESS_SIZE*2+1];
    std::string Name;
    std::string StatusMessage;
    TOX_USER_STATUS Status;
    std::vector<toxFriend> friendVector;
public:
    pTox() {}
    pTox(bool newAccount, std::string FilePath);
    ~pTox();
    std::string getConfiguration();

    size_t friendVectorSize();
    toxFriend friendVectorData(size_t);
    toxFriend friendVectorData(uint32_t);
    bool isFriendVector(uint32_t);
    std::string friendName(uint32_t);
    TOX_USER_STATUS friendStatus(uint32_t);

    void updateToxFriendlList();
signals:
    void changeTable();
    void appendText(QString);
    void friendRequestRecived(std::string, QString);

    void newAudioCall(uint32_t);
    void endAudioCall(uint32_t);
    void revivedAudioFrame(uint32_t friend_number, const int16_t *pcm, size_t sample_count, uint8_t channels, uint32_t sampling_rate);
public slots:
    void setName(std::string Name);
    void setStatus(TOX_USER_STATUS status);
    void setStatus(std::string StatusMsg);

    void addFriend(uint32_t);
    void removeFriend(uint32_t);
    void clearFriendVector();
    void sendRequest(std::string Address, std::string Message);
    void decisionRequest(std::string Address, bool decision);

    void sendMessage(std::string);
    void sendMessage(uint32_t, std::string);

    void audioCall(uint32_t, bool);
    void sendAudioFrame(uint32_t friend_number, const int16_t *pcm, size_t sample_count, uint8_t channels, uint32_t sampling_rate);

protected:
    std::string uint82string(uint8_t *tab, size_t lenght);
    uint8_t *bin2hex(uint8_t * const hex, const size_t hex_maxlen, const uint8_t * const bin, const size_t bin_len);
    std::string hex2bin(const std::string &value);
    int hex2bin(unsigned char *const bin, const size_t bin_maxlen, const char *const hex, const size_t hex_len, const char *const ignore, size_t *const bin_len, const char **const hex_end);
    std::string Status2String(TOX_USER_STATUS s);
    std::string Connection2String(TOX_CONNECTION c);
    bool CreateNewAccount();
    bool ReadFile();
    bool CreateFile();
    bool SaveProfile();
    void Connect();

    static void *runToxThread(void *arg);
    static void *runToxAVThread(void *arg);
private:
    /*--- CALLBACKS ---*/
    /**
    * @param connection_status Whether we are connected to the DHT.
    */
    static void callback_self_connection_status(Tox *tox, TOX_CONNECTION status, void *userData);
    /**
    * @param friend_number The friend number of the friend whose connection status changed.
    * @param connection_status The result of calling tox_friend_get_connection_status on the passed friend_number.
    */
    static void callback_friend_connection_status(Tox *tox, uint32_t friend_number, TOX_CONNECTION connection_status, void *user_data);
    /**
    * @param friend_number The friend number of the friend who sent the message.
    * @param message The message data they sent.
    * @param length The size of the message byte array.
    */
    static void callback_friend_message(Tox *tox, uint32_t friend_number, TOX_MESSAGE_TYPE type, const uint8_t *message, size_t length, void *user_data);
    /**
    * @param public_key The Public Key of the user who sent the friend request.
    * @param message The message they sent along with the request.
    * @param length The size of the message byte array.
    */
    static void callback_friend_request(Tox *tox, const uint8_t *public_key, const uint8_t *message, size_t length, void *user_data);
    /**
    * @param friend_number The friend number of the friend whose name changed.
    * @param name A byte array containing the same data as tox_friend_get_name would write to its `name` parameter.
    * @param length A value equal to the return value of tox_friend_get_name_size.
    */
    static void callback_friend_name(Tox *tox, uint32_t friend_number, const uint8_t *name, size_t length, void *user_data);
    /**
    * @param friend_number The friend number of the friend whose status message changed.
    * @param message A byte array containing the same data as tox_friend_get_status_message would write to its `status_message` parameter.
    * @param length A value equal to the return value of tox_friend_get_status_message_size.
    */
    static void callback_friend_status_message(Tox *tox, uint32_t friend_number, const uint8_t *message, size_t length, void *user_data);
    /**
    * @param friend_number The friend number of the friend whose user status changed.
    * @param status The new user status.
    */
    static void callback_friend_status(Tox *tox, uint32_t friend_number, TOX_USER_STATUS status, void *user_data);
    /**
    * The function type for the call callback.
    * @param friend_number The friend number from which the call is incoming.
    * @param audio_enabled True if friend is sending audio.
    * @param video_enabled True if friend is sending video.
    */
    static void callback_call(ToxAV *av, uint32_t friend_number, bool audio_enabled, bool video_enabled, void *user_data);
    /**
    * The function type for the call_state callback.
    * @param friend_number The friend number for which the call state changed.
    * @param state The bitmask of the new call state which is guaranteed to be
    * different than the previous state. The state is set to 0 when the call is
    * paused. The bitmask represents all the activities currently performed by the friend.
    */
    static void callback_call_state(ToxAV *av, uint32_t friend_number, uint32_t state, void *user_data);
    /**
    * The function type for the audio_receive_frame callback. The callback can be
    * called multiple times per single iteration depending on the amount of queued
    * frames in the buffer. The received format is the same as in send function.
    * @param friend_number The friend number of the friend who sent an audio frame.
    * @param pcm An array of audio samples (sample_count * channels elements).
    * @param sample_count The number of audio samples per channel in the PCM array.
    * @param channels Number of audio channels.
    * @param sampling_rate Sampling rate used in this frame.
    */
    static void callback_audio_receive_frame(ToxAV *av, uint32_t friend_number, const int16_t *pcm, size_t sample_count, uint8_t channels, uint32_t sampling_rate, void *user_data);
    /**
    * The client should acquire resources to be associated with the file transfer.
    * Incoming file transfers start in the PAUSED state. After this callback
    * returns, a transfer can be rejected by sending a TOX_FILE_CONTROL_CANCEL
    * control command before any other control commands. It can be accepted by
    * sending TOX_FILE_CONTROL_RESUME.
    * @param friend_number The friend number of the friend who is sending the file transfer request.
    * @param file_number The friend-specific file number the data received is associated with.
    * @param kind The meaning of the file to be sent.
    * @param file_size Size in bytes of the file the client wants to send, UINT64_MAX if unknown or streaming.
    * @param filename Name of the file. Does not need to be the actual name. This name will be sent along with the file send request.
    * @param filename_length Size in bytes of the filename.
    */
    static void callback_file_recv(Tox *tox, uint32_t friend_number, uint32_t file_number, uint32_t kind, uint64_t file_size, const uint8_t *filename, size_t filename_length, void *user_data);
//    /**
//    * @param friend_number The friend number of the receiving friend for this file.
//    * @param file_number The file transfer identifier returned by tox_file_send.
//    * @param position The file or stream position from which to continue reading.
//    * @param length The number of bytes requested for the current chunk.
//    */
//    static void callback_file_chunk_request(Tox *tox, uint32_t friend_number, uint32_t file_number, uint64_t position, size_t length, void *user_data);
//    /**
//    * @param friend_number The friend number of the friend who is sending the file.
//    * @param file_number The friend-specific file number the data received is
//    *   associated with.
//    * @param control The file control command received.
//    */
//    static void callback_file_recv_control(Tox *tox, uint32_t friend_number, uint32_t file_number, TOX_FILE_CONTROL control, void *user_data);
//    /**
//    * @param friend_number The friend number of the friend who is sending the file.
//    * @param file_number The friend-specific file number the data received is associated with.
//    * @param position The file position of the first byte in data.
//    * @param data A byte array containing the received chunk.
//    * @param length The length of the received chunk.
//     */
//    static void callback_file_recv_chunk(Tox *tox, uint32_t friend_number, uint32_t file_number, uint64_t position, const uint8_t *data, size_t length, void *user_data);

    /*--- ERRORS ---*/
    std::string tox_set_info_error(TOX_ERR_SET_INFO error);
    std::string tox_add_friend_error(TOX_ERR_FRIEND_ADD error);
    std::string tox_friend_querry_error(TOX_ERR_FRIEND_QUERY error);
    std::string tox_send_message_error(TOX_ERR_FRIEND_SEND_MESSAGE error);
    std::string toxav_new_error(TOXAV_ERR_NEW error);
    std::string toxav_call_error(TOXAV_ERR_CALL_CONTROL error);
    std::string toxav_send_frame_error(TOXAV_ERR_SEND_FRAME error);
    std::string toxav_answer_error(TOXAV_ERR_ANSWER error);
};

#endif // PTOX_H
