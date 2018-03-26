#include "ptox.h"

pTox *PTOX = nullptr;

pTox::pTox(bool newAccount, std::string FilePath)
{
    this->FilePath = FilePath;

    tox_options_default(&toxOptions);
    if (newAccount) {
        std::cout << "New Account" << std::endl;
        CreateNewAccount();
    } else {
        std::cout << "Read file" << std::endl;
        ReadFile();
    }

    tox_callback_self_connection_status(tox, callback_self_connection_status);
    tox_callback_friend_request(tox, callback_friend_request);
    tox_callback_friend_message(tox, callback_friend_message);
    tox_callback_file_recv(tox, callback_file_recv);
//    tox_callback_file_chunk_request(tox, callback_file_chunk_request);
//    tox_callback_file_recv_control(tox, callback_file_recv_control);
//    tox_callback_file_recv_chunk(tox, callback_file_recv_chunk);

    framesize = (AUDIOSAMPLERATE * AUDIOFRAME * AUDIOCHANNELS) / 1000;

    tox_self_get_address(tox, (uint8_t*)AddressBin);

    bin2hex(AddressHex, sizeof(AddressHex), AddressBin, sizeof(AddressBin));

    std::cout << "Address Hex: " << AddressHex << std::endl;

    Connect();

    //ToxAV
    TOXAV_ERR_NEW errorav;
    toxav = toxav_new(tox, &errorav);
    if (errorav != TOXAV_ERR_NEW_OK) {
        std::cerr << "ToxAV Init fail!: " << errorav << std::endl;
        return;
    }
    toxav_callback_call(toxav, callback_call, NULL);
    toxav_callback_call_state(toxav, callback_call_state, NULL);
    toxav_callback_audio_receive_frame(toxav, callback_audio_receive_frame, NULL);

    //threads
    pthread_create(&tox_thread, NULL, &runToxThread, tox);
    pthread_create(&toxav_thread, NULL, &runToxAVThread, toxav);

    PTOX = this;

    updateToxFriendlList();
}

pTox::~pTox()
{
    pthread_cancel(tox_thread);
    pthread_cancel(toxav_thread);

    SaveProfile();

    this->friendVector.clear();


    toxav_kill(toxav);
    tox_kill(tox);

    std::cout << "GOODBYE WORLD" << std::endl;
}

std::string pTox::getConfiguration()
{
    size_t nameSize = tox_self_get_name_size(tox);
    size_t status_messageSize = tox_self_get_status_message_size(tox);
    unsigned char *name = new unsigned char[nameSize+1];
    unsigned char *status_message = new unsigned char[status_messageSize+1];
    tox_self_get_name(tox, (uint8_t*)name);
    tox_self_get_status_message(tox, (uint8_t*)status_message);
    name[nameSize] = '\0';
    status_message[status_messageSize] = '\0';
    TOX_USER_STATUS Status = tox_self_get_status(tox);
    std::cout << "Name: " << name << std::endl;
    std::cout << "Status: " << Status2String(Status) << " : " << status_message << std::endl;


    delete [] name;
    delete [] status_message;

    std::stringstream ss;
    ss << "Name:       <b>" << this->Name << "</b><br />";
    ss << "Address:    <b>" << this->AddressHex << "</b><br />";
    ss << "Status:     <b>" << Status2String(this->Status) <<"</b><br />";
    ss << "Status MSG: <b>" << this->StatusMessage << "</b><br />";

    return ss.str();
}

size_t pTox::friendVectorSize()
{
    return this->friendVector.size();
}

pTox::toxFriend pTox::friendVectorData(size_t i)
{
    return this->friendVector.at(i);
}

pTox::toxFriend pTox::friendVectorData(uint32_t friendNumber)
{
    for (toxFriend f : this->friendVector) {
        if (f.friendNumber == friendNumber) {
            return f;
        }
    }
    throw std::invalid_argument("Friend not found");
}

bool pTox::isFriendVector(uint32_t friendNumber)
{
    for (toxFriend f : this->friendVector) {
        if (f.friendNumber == friendNumber) {
            return true;
        }
    }
    return false;
}

std::string pTox::friendName(uint32_t friendNumber)
{
    for (toxFriend f : this->friendVector) {
        if (f.friendNumber == friendNumber) {
            return f.friendName;
        }
    }
    return NULL;
}

TOX_USER_STATUS pTox::friendStatus(uint32_t friendNumber)
{
    for (toxFriend f : this->friendVector) {
        if (f.friendNumber == friendNumber) {
            return f.friendConnectionStatus;
        }
    }
    return TOX_USER_STATUS_AWAY;
}


void pTox::setName(std::string Name)
{
    this->Name = Name;

    TOX_ERR_SET_INFO error;
    tox_self_set_name(tox, (uint8_t*)(Name.c_str()), Name.size(), &error);
    if (error != TOX_ERR_SET_INFO_OK) {
        std::cerr << "ERROR setName: " << tox_set_info_error(error) << std::endl;
    }
}

void pTox::setStatus(TOX_USER_STATUS status)
{
    this->Status = status;
    tox_self_set_status(tox, (TOX_USER_STATUS) status);
}

void pTox::setStatus(std::string StatusMsg)
{
    TOX_ERR_SET_INFO error;
    this->StatusMessage = StatusMsg;

    tox_self_set_status_message(tox, (uint8_t*)(StatusMsg.c_str()), StatusMsg.size(), &error);

    if (error != TOX_ERR_SET_INFO_OK) {
        std::cerr << "ERROR setStatus: " << tox_set_info_error(error) << std::endl;
    }
}

void pTox::updateToxFriendlList()
{
    friendVector.clear();
    size_t ListSize = tox_self_get_friend_list_size(tox);
    uint32_t *FriendList = new uint32_t[ListSize];
    tox_self_get_friend_list(tox, FriendList);
    for (size_t i = 0; i < ListSize; ++i) {
        struct toxFriend f;
        TOX_ERR_FRIEND_QUERY error;
        size_t friendNameSize = tox_friend_get_name_size(tox, FriendList[i], &error);
        if (error != TOX_ERR_FRIEND_QUERY_OK) {
            std::cerr << "ERROR CreateFriend: " << tox_friend_querry_error(error) << std::endl;
            return;
        } else if (friendNameSize == 0) {
            f.friendName = "Anonymous";
        } else {
            uint8_t tempName[TOX_MAX_NAME_LENGTH+1];
            tox_friend_get_name(tox, FriendList[i], tempName, &error);
            f.friendName = reinterpret_cast<char const*>(tempName);
            f.friendName += '\0';
        }

        f.friendNumber = FriendList[i];
        TOX_ERR_FRIEND_GET_PUBLIC_KEY error2;
        if (!tox_friend_get_public_key(tox, f.friendNumber, f.friendAddressHex, &error2)) {
            std::cerr << "ERROR CreateFriend get public key: " << f.friendName << " ERROR: " << error2 << std::endl;
            return;
        } else {
            hex2bin(f.friendAddressBin, sizeof(f.friendAddressBin), (const char*)f.friendAddressHex, sizeof(f.friendAddressHex), NULL, NULL, NULL);
        }
        friendVector.push_back(f);

    }

    delete [] FriendList;

    emit changeTable();
}

std::string pTox::Status2String(TOX_USER_STATUS s)
{
    switch (s) {
    case TOX_USER_STATUS_NONE:
        return "AVAILABLE";
    case TOX_USER_STATUS_AWAY:
        return "AWAY";
    case TOX_USER_STATUS_BUSY:
        return "BUSY";
    default:
        return "unknown";
    }
}


void pTox::addFriend(uint32_t friendNumber)
{
    struct toxFriend f;
    TOX_ERR_FRIEND_QUERY error;
    size_t friendNameSize = tox_friend_get_name_size(tox, friendNumber, &error);
    if (error != TOX_ERR_FRIEND_QUERY_OK) {
        std::cerr << "ERROR addFriend friendNameSize: " << tox_friend_querry_error(error) << std::endl;
        return;
    } else if (friendNameSize == 0) {
        f.friendName = "Anonymous";
    } else {
        uint8_t tempName[TOX_MAX_NAME_LENGTH+1];
        tox_friend_get_name(tox, friendNumber, tempName, &error);
        if (error != TOX_ERR_FRIEND_QUERY_OK) {
            std::cerr << "ERROR addFriend get name: " << tox_friend_querry_error(error) << std::endl;
            return;
        }
        f.friendName = reinterpret_cast<char const*>(tempName);
        f.friendName += '\0';
    }

    f.friendConnectionStatus = tox_friend_get_status(tox, friendNumber, &error);
    if (error != TOX_ERR_FRIEND_QUERY_OK) {
        std::cerr << "ERROR addFriend Connection status: " << tox_friend_querry_error(error) << std::endl;
    }

    f.friendNumber = friendNumber;
    TOX_ERR_FRIEND_GET_PUBLIC_KEY error2;
    if (!tox_friend_get_public_key(tox, f.friendNumber, f.friendAddressHex, &error2)) {
        std::cerr << "ERROR addFriend get public key: " << f.friendName << "  No friend with the given number exists on the friend list. " << std::endl;
        return;
    } else {
        hex2bin(f.friendAddressBin, sizeof(f.friendAddressBin), (const char*)f.friendAddressHex, sizeof(f.friendAddressHex), NULL, NULL, NULL);
    }
    friendVector.push_back(f);

    SaveProfile();
    updateToxFriendlList();
}

void pTox::sendRequest(std::string Address, std::string Message)
{
    TOX_ERR_FRIEND_ADD error;
    std::string toxid = hex2bin(Address);

    uint32_t FriendNumber = tox_friend_add(tox, (const uint8_t*) toxid.c_str(), (const uint8_t*) Message.c_str(), Message.size(), &error);
    if (error != TOX_ERR_FRIEND_ADD_OK) {
        std::cerr << "ERROR Friend request: " << Address << " error: " << tox_add_friend_error(error) << std::endl;
        return;
    }
    addFriend(FriendNumber);
}

void pTox::decisionRequest(std::string Address, bool decision)
{
    std::string toxid = hex2bin(Address);
    if (decision) {
        TOX_ERR_FRIEND_ADD error;
        uint32_t FriendNumber = tox_friend_add_norequest(tox, (const uint8_t*)toxid.c_str(), &error);
        if (error != TOX_ERR_FRIEND_ADD_OK) {
            std::cerr << "ERROR Friend request: " << Address << " error: " << tox_add_friend_error(error) << std::endl;
            return;
        }
        addFriend(FriendNumber);
    } else {
        TOX_ERR_FRIEND_BY_PUBLIC_KEY error;
        uint32_t FriendNumber = tox_friend_by_public_key(tox, (const uint8_t*)toxid.c_str(), &error);
        if (error == TOX_ERR_FRIEND_BY_PUBLIC_KEY_OK) {
            if (!tox_friend_delete(tox, FriendNumber, NULL)) {
                std::cerr << "ERROR delete friend!"  << std::endl;
            }
        } //else {} //There is no such friend don't have to delete him
    }
}

void pTox::removeFriend(uint32_t friendNumber)
{
    TOX_ERR_FRIEND_DELETE error;
    tox_friend_delete(tox, friendNumber, &error);
    if (error != TOX_ERR_FRIEND_DELETE_OK) {
        std::cerr << "ERROR remove friend - Friend not found" << std::endl;
    }

    updateToxFriendlList();
    SaveProfile();
}

void pTox::clearFriendVector()
{
    friendVector.clear();

    size_t ListSize = tox_self_get_friend_list_size(tox);
    uint32_t *FriendList = new uint32_t[ListSize];

    tox_self_get_friend_list(tox, FriendList);

    for (size_t i = 0; i < ListSize; ++i) {
        struct toxFriend f;
        TOX_ERR_FRIEND_QUERY error;
        size_t friendNameSize = tox_friend_get_name_size(tox, FriendList[i], &error);
        if (error != TOX_ERR_FRIEND_QUERY_OK) {
            std::cerr << "ERROR CreateFriend: " << tox_friend_querry_error(error) << std::endl;
            continue;
        } else if (friendNameSize == 0) {
            f.friendName = "Anonymous";
        } else {
            uint8_t tempName[TOX_MAX_NAME_LENGTH+1];
            tox_friend_get_name(tox, FriendList[i], tempName, &error);
            f.friendName = reinterpret_cast<char const*>(tempName);
            f.friendName += '\0';
        }

        f.friendNumber = FriendList[i];
        if (!tox_friend_get_public_key(tox, f.friendNumber, f.friendAddressHex, NULL)) {
            std::cerr << "ERROR CreateFriend get public key: " << f.friendName << std::endl;
            continue;
        } else {
            hex2bin(f.friendAddressBin, sizeof(f.friendAddressBin), (const char*)f.friendAddressHex, sizeof(f.friendAddressHex), NULL, NULL, NULL);
        }

        if (f.friendAddressBin == 0) {
            for (size_t j = 0; j < friendVector.size(); ++j) {
                struct toxFriend FF = friendVector[i];
                if (f.friendNumber == FF.friendNumber) {
                    friendVector.erase(friendVector.begin()+i);
                }
            }
        } else {
            continue;
        }
    }

    delete [] FriendList;
    updateToxFriendlList();
    SaveProfile();
}

void pTox::sendMessage(std::string msg)
{
    TOX_ERR_FRIEND_SEND_MESSAGE error;
    for (toxFriend f : friendVector) {
        tox_friend_send_message(tox, f.friendNumber, TOX_MESSAGE_TYPE_NORMAL, (uint8_t*) msg.c_str(), msg.size(), &error);
        if (error != TOX_ERR_FRIEND_SEND_MESSAGE_OK) {
            std::cerr << "ERROR: sendMessage to: " << f.friendName << " " << tox_send_message_error(error) << std::endl;
        }
    }
}

void pTox::sendMessage(uint32_t friendNumber, std::string msg)
{
    TOX_ERR_FRIEND_SEND_MESSAGE error;
    tox_friend_send_message(tox, friendNumber, TOX_MESSAGE_TYPE_NORMAL, (uint8_t*) msg.c_str(), msg.size(), &error);
    if (error != TOX_ERR_FRIEND_SEND_MESSAGE_OK) {
        std::cerr << "ERROR: sendMessage to: " << friendName(friendNumber) << " " << tox_send_message_error(error) << std::endl;
    }
}

void pTox::audioCall(uint32_t friend_number, bool decision)
{
    TOXAV_ERR_ANSWER error;
    if (decision) {
        toxav_answer(toxav, friend_number, AUDIOBITRATE, 0, &error);
    } else {
        toxav_answer(toxav, friend_number, 0, 0, &error);
    }
    if (error != TOXAV_ERR_ANSWER_OK)
        std::cerr << "ERROR Toxav answer: " << toxav_answer_error(error) << std::endl;
}

void pTox::sendAudioFrame(uint32_t friend_number, const int16_t *pcm, size_t sample_count, uint8_t channels, uint32_t sampling_rate)
{
    TOXAV_ERR_SEND_FRAME error;
    toxav_audio_send_frame(toxav, friend_number, pcm, sample_count, channels, sampling_rate, &error);

    if (error != TOXAV_ERR_SEND_FRAME_OK) {
        std::cerr << "ERROR: send audio frame to: " << friendName(friend_number) << " error: " << toxav_send_frame_error(error) << std::endl;
    }
}

std::string pTox::uint82string(uint8_t *tab, size_t lenght)
{
    std::string s;
    s.reserve(lenght+1);
    for (size_t i = 0; i < lenght; ++i) {
        s += (char)tab[i];
    }

    return s;
}

uint8_t *pTox::bin2hex(uint8_t * const hex, const size_t hex_maxlen, const uint8_t * const bin, const size_t bin_len)
{
    size_t       i = static_cast<size_t>(0U);
    unsigned int x = 0;
    int          b = 0;
    int          c = 0;

    if (bin_len >= SIZE_MAX / 2 || hex_maxlen <= bin_len * 2U)
        abort(); /* LCOV_EXCL_LINE */
    while (i < bin_len)
    {
        c = bin[i] & 0xf;
        b = bin[i] >> 4;
        x = static_cast<uint8_t>(87U + c + (((c - 10U) >> 8) & ~38U)) << 8 | static_cast<uint8_t>(87U + b + (((b - 10U) >> 8) & ~38U));
        hex[i * 2U] = static_cast<char>(x);
        x >>= 8;
        hex[i * 2U + 1U] = static_cast<char>(x);
        i++;
    }
    hex[i * 2U] = 0U;

    return hex;
}

std::string pTox::hex2bin(const std::string &value)
{
    size_t sz = value.size() / 2;
    std::string r;
    r.resize(sz);
    hex2bin((unsigned char *)r.c_str(), sz, value.c_str(), sz*2, NULL, NULL, NULL);

    return r;
}

int pTox::hex2bin(unsigned char * const bin, const size_t bin_maxlen, const char * const hex, const size_t hex_len, const char * const ignore, size_t * const bin_len, const char ** const hex_end)
{
    size_t        bin_pos = (size_t) 0U;
    size_t        hex_pos = (size_t) 0U;
    int           ret     = 0;
    unsigned char c;
    unsigned char c_acc = 0U;
    unsigned char c_alpha0, c_alpha;
    unsigned char c_num0, c_num;
    unsigned char c_val;
    unsigned char state = 0U;

    while (hex_pos < hex_len) {
        c        = (unsigned char) hex[hex_pos];
        c_num    = c ^ 48U;
        c_num0   = (c_num - 10U) >> 8;
        c_alpha  = (c & ~32U) - 55U;
        c_alpha0 = ((c_alpha - 10U) ^ (c_alpha - 16U)) >> 8;
        if ((c_num0 | c_alpha0) == 0U) {
            if (ignore != NULL && state == 0U && strchr(ignore, c) != NULL) {
                hex_pos++;
                continue;
            }
            break;
        }
        c_val = (c_num0 & c_num) | (c_alpha0 & c_alpha);
        if (bin_pos >= bin_maxlen) {
            ret   = -1;
            errno = ERANGE;
            break;
        }
        if (state == 0U) {
            c_acc = c_val * 16U;
        } else {
            bin[bin_pos++] = c_acc | c_val;
        }
        state = ~state;
        hex_pos++;
    }
    if (state != 0U) {
        hex_pos--;
        errno = EINVAL;
        ret = -1;
    }
    if (ret != 0) {
        bin_pos = (size_t) 0U;
    }
    if (hex_end != NULL) {
        *hex_end = &hex[hex_pos];
    } else if (hex_pos != hex_len) {
        errno = EINVAL;
        ret = -1;
    }
    if (bin_len != NULL) {
        *bin_len = bin_pos;
    }
    return ret;
}

std::string pTox::Connection2String(TOX_CONNECTION c)
{
    switch (c) {
    case TOX_CONNECTION_NONE:
        return "OFFLINE";
    case TOX_CONNECTION_TCP:
    case TOX_CONNECTION_UDP:
        return "ONLINE";
    default:
        return "unknown";
    }
}

bool pTox::CreateNewAccount()
{
    TOX_ERR_NEW error;
    tox = tox_new(&toxOptions, &error);
    if (error != TOX_ERR_NEW_OK) {
        std::cerr << "Tox Init fail: " << error << std::endl;
        return false;
    }

    if (!SaveProfile()) {
        std::cerr << "Tox Init failed save profile! " << std::endl;
        return false;
    }

    return true;
}

bool pTox::ReadFile()
{
    FILE *file = fopen(FilePath.c_str(), "rb");
    if (file) {
        fseek(file, 0, SEEK_END);
        size_t FileSize = ftell(file);
        fseek(file, 0, SEEK_SET);

        uint8_t* FileData = new uint8_t[FileSize];

        fread(FileData, sizeof(uint8_t), FileSize, file);
        fclose(file);

        toxOptions.savedata_type = TOX_SAVEDATA_TYPE_TOX_SAVE;
        toxOptions.savedata_data = FileData;
        toxOptions.savedata_length = FileSize;


        TOX_ERR_NEW error;
        tox = tox_new(&toxOptions, &error);

        free(FileData);

        return error == TOX_ERR_NEW_OK;
    }

    return false;
}

bool pTox::CreateFile()
{
    tox_options_default(&toxOptions);
    tox = tox_new(&toxOptions, NULL);
    SaveProfile();
    return true;
}

bool pTox::SaveProfile()
{
    size_t Size = tox_get_savedata_size(tox);
    uint8_t Data[Size];

    tox_get_savedata(tox, Data);

    FILE *file = fopen(FilePath.c_str(), "wb+");
    if (file) {
        fwrite(Data, sizeof(uint8_t), Size, file);
        fclose(file);
        return true;
    } else {
        std::cerr << "ERROR WHILE SAVING DATA!" << std::endl;
        return false;
    }
}

void pTox::Connect()
{
    struct node *n;
    TOX_ERR_BOOTSTRAP error;
    for (unsigned int i = 0; i < LEN(nodes); ++i) {
        n = &nodes[i];
        uint8_t key_bin[TOX_PUBLIC_KEY_SIZE];
        hex2bin(key_bin, sizeof(key_bin), n->key, sizeof(n->key), NULL, NULL, NULL);
        tox_bootstrap(tox, n->addr4, n->port, key_bin, &error);
        if (error != TOX_ERR_BOOTSTRAP_OK) {
            std::cerr << "ERROR BOOTSTRAP: " << n->addr4 << " - " << error << std::endl;
        }
    }
}

void *pTox::runToxThread(void *arg)
{
    Tox *t_tox = (Tox*)arg;
    while (true) {
        tox_iterate(t_tox, NULL);

        uint32_t time = tox_iteration_interval(t_tox);
        std::this_thread::sleep_for(std::chrono::milliseconds(time));
    }
}

void *pTox::runToxAVThread(void *arg)
{
    ToxAV *t_toxav = (ToxAV*)arg;
    while (true) {
        toxav_iterate(t_toxav);

        uint32_t time = toxav_iteration_interval(t_toxav);
        std::this_thread::sleep_for(std::chrono::milliseconds(time));
    }
}

void pTox::callback_self_connection_status(Tox *tox, TOX_CONNECTION status, void *userData)
{
    if (status == TOX_CONNECTION_NONE) {
        std::cerr << "Error with the connection!" << std::endl;
        emit PTOX->appendText("<font color=\"red\">ERROR with the connection!</font><br />");
        emit PTOX->changeTable();
    } else {
        std::cout << "Connected to TOX, status: " << status << std::endl;
        emit PTOX->appendText("<font color=\"green\">Sucessfully connected to TOX!</font><br />");
        emit PTOX->changeTable();
    }
}

void pTox::callback_friend_connection_status(Tox *tox, uint32_t friend_number, TOX_CONNECTION connection_status, void *user_data)
{
    std::cout << "Friend connection status changed: " << PTOX->friendName(friend_number) << " to: " << PTOX->Connection2String(connection_status) << std::endl;
    PTOX->updateToxFriendlList();
    emit PTOX->appendText(QString("Friend <b>%1</b>changed connection status for: %2!").arg(QString::fromStdString(PTOX->friendName(friend_number))).arg(QString::fromStdString(PTOX->Connection2String(connection_status))));
}

void pTox::callback_friend_message(Tox *tox, uint32_t friend_number, TOX_MESSAGE_TYPE type, const uint8_t *message, size_t length, void *user_data)
{
    std::cout << "Friend " << PTOX->friendName(friend_number) << " messaged: " << message << std::endl;
    emit PTOX->appendText(QString("Friend: <b>%1</b> wrote: %2").arg(QString::fromStdString(PTOX->friendName(friend_number))).arg(QString::fromLocal8Bit((char*)message, length)));
}

void pTox::callback_friend_request(Tox *tox, const uint8_t *public_key, const uint8_t *message, size_t length, void *user_data)
{
    std::cout << "Maybe new friend send request: " << message << std::endl;;
    QString temp = QString::fromLocal8Bit((char*)public_key, TOX_PUBLIC_KEY_SIZE);
    emit PTOX->friendRequestRecived(temp.toStdString(), QString::fromLocal8Bit((char*)message, length));
}

void pTox::callback_friend_name(Tox *tox, uint32_t friend_number, const uint8_t *name, size_t length, void *user_data)
{
    std::cout << "Friend " << PTOX->friendName(friend_number) << " changed name!" << std::endl;
    PTOX->updateToxFriendlList();
    emit PTOX->appendText(QString("Friend <b>%1</b>changed name for: %2!").arg(QString::fromStdString(PTOX->friendName(friend_number))).arg(QString::fromLocal8Bit((char*)name, length)));
}

void pTox::callback_friend_status_message(Tox *tox, uint32_t friend_number, const uint8_t *message, size_t length, void *user_data)
{
    std::cout << "Friend " << PTOX->friendName(friend_number) << " changed status for: " << message << std::endl;
    PTOX->updateToxFriendlList();
    emit PTOX->appendText(QString("Friend <b>%1</b>changed status for: %2!").arg(QString::fromStdString(PTOX->friendName(friend_number))).arg(QString::fromLocal8Bit((char*)message, length)));
}

void pTox::callback_friend_status(Tox *tox, uint32_t friend_number, TOX_USER_STATUS status, void *user_data)
{
    std::cout << "Friend " << PTOX->friendName(friend_number) << " changed status for: " << PTOX->Status2String(status) << std::endl;
    PTOX->updateToxFriendlList();
    emit PTOX->appendText(QString("Friend <b>%1</b>changed status for: %2!").arg(QString::fromStdString(PTOX->friendName(friend_number))).arg(QString::fromStdString(PTOX->Status2String(status))));
}

void pTox::callback_call(ToxAV *av, uint32_t friend_number, bool audio_enabled, bool video_enabled, void *user_data)
{
    //Sb is inviting for a call
    if (!audio_enabled) {
        TOXAV_ERR_CALL_CONTROL error;
        if (!toxav_call_control(av, friend_number, TOXAV_CALL_CONTROL_CANCEL, &error)) {
            std::cerr << "Failed to reject call! error: " << PTOX->toxav_call_error(error) << " from: " << PTOX->friendName(friend_number) << std::endl;
            return;
        } else if (video_enabled) {
            std::cout << "Friend: " << PTOX->friendName(friend_number) << " wants to connect via video - rejected!" << std::endl;
            return;
        }
    }
    emit PTOX->newAudioCall(friend_number);
}

void pTox::callback_call_state(ToxAV *av, uint32_t friend_number, uint32_t state, void *user_data)
{
    //Change for call status
    toxFriend f = PTOX->friendVectorData(friend_number);
    if (state & TOXAV_FRIEND_CALL_STATE_FINISHED) {
        std::cout << "Call with friend: " << PTOX->friendName(friend_number) << " Finished!" << std::endl;
        emit PTOX->endAudioCall(friend_number);
    } else if (state & TOXAV_FRIEND_CALL_STATE_ERROR) {
        std::cout << "Call with friend: " << PTOX->friendName(friend_number) << " was ended - error happend!" << std::endl;
        emit PTOX->endAudioCall(friend_number);
    }
}

void pTox::callback_audio_receive_frame(ToxAV *av, uint32_t friend_number, const int16_t *pcm, size_t sample_count, uint8_t channels, uint32_t sampling_rate, void *user_data)
{
    emit PTOX->revivedAudioFrame(friend_number, pcm, sample_count, channels, sampling_rate);
}

void pTox::callback_file_recv(Tox *tox, uint32_t friend_number, uint32_t file_number, uint32_t kind, uint64_t file_size, const uint8_t *filename, size_t filename_length, void *user_data)
{
    if (kind == TOX_FILE_KIND_AVATAR) {
        return;
    }

    tox_file_control(tox, friend_number, file_number, TOX_FILE_CONTROL_CANCEL, NULL);

    const char *MSG = "Sorry, I don't support file transfers, YET!";
    tox_friend_send_message(tox, friend_number, TOX_MESSAGE_TYPE_NORMAL, (uint8_t*)MSG, strlen(MSG), NULL);

    if (PTOX) {
        emit PTOX->appendText(QString("Friend: <b>%1</b> wanted to send file!").arg(QString::fromStdString(PTOX->friendName(friend_number))));
    }
}

//void pTox::callback_file_chunk_request(Tox *tox, uint32_t friend_number, uint32_t file_number, uint64_t position, size_t length, void *user_data)
//{

//}

//void pTox::callback_file_recv_control(Tox *tox, uint32_t friend_number, uint32_t file_number, TOX_FILE_CONTROL control, void *user_data)
//{

//}

//void pTox::callback_file_recv_chunk(Tox *tox, uint32_t friend_number, uint32_t file_number, uint64_t position, const uint8_t *data, size_t length, void *user_data)
//{

//}

std::string pTox::tox_set_info_error(TOX_ERR_SET_INFO error)
{
    switch (error) {
    case TOX_ERR_SET_INFO_OK:
        return "The function returned successfully.";
    case TOX_ERR_SET_INFO_NULL:
        return "One of the arguments to the function was NULL when it was not expected.";
    case TOX_ERR_SET_INFO_TOO_LONG:
        return "Information length exceeded maximum permissible size.";
    default:
        return "Unknown error";
    }
}

std::string pTox::tox_add_friend_error(TOX_ERR_FRIEND_ADD error)
{
    switch (error) {
    case TOX_ERR_FRIEND_ADD_OK:
        return "The function returned successfully.";
    case TOX_ERR_FRIEND_ADD_NULL:
        return "One of the arguments to the function was NULL when it was not expected.";
    case TOX_ERR_FRIEND_ADD_TOO_LONG:
        return "The length of the friend request message exceeded TOX_MAX_FRIEND_REQUEST_LENGTH.";
    case TOX_ERR_FRIEND_ADD_NO_MESSAGE:
        return "The friend request message was empty. This, and the TOO_LONG code will never be returned from tox_friend_add_norequest.";
    case TOX_ERR_FRIEND_ADD_OWN_KEY:
        return "The friend address belongs to the sending client.";
    case TOX_ERR_FRIEND_ADD_ALREADY_SENT:
        return "A friend request has already been sent, or the address belongs to a friend that is already on the friend list.";
    case TOX_ERR_FRIEND_ADD_BAD_CHECKSUM:
        return "The friend address checksum failed.";
    case TOX_ERR_FRIEND_ADD_SET_NEW_NOSPAM:
        return "The friend was already there, but the nospam value was different.";
    case TOX_ERR_FRIEND_ADD_MALLOC:
        return "A memory allocation failed when trying to increase the friend list size.";
    default:
        return "Unknown error";
    }
}

std::string pTox::tox_friend_querry_error(TOX_ERR_FRIEND_QUERY error)
{
    switch (error) {
    case TOX_ERR_FRIEND_QUERY_OK:
        return "The function returned successfully.";
    case TOX_ERR_FRIEND_QUERY_NULL:
        return "The pointer parameter for storing the query result (name, message) was NULL. Unlike the `_self_` variants of these functions, which have no effect when a parameter is NULL, these functions return an error in that case.";
    case TOX_ERR_FRIEND_QUERY_FRIEND_NOT_FOUND:
        return "The friend_number did not designate a valid friend.";
    default:
        return "Unknown error";
    }
}

std::string pTox::tox_send_message_error(TOX_ERR_FRIEND_SEND_MESSAGE error)
{
    switch (error) {
    case TOX_ERR_FRIEND_SEND_MESSAGE_OK:
        return "The function returned successfully.";
    case TOX_ERR_FRIEND_SEND_MESSAGE_NULL:
        return "One of the arguments to the function was NULL when it was not expected.";
    case TOX_ERR_FRIEND_SEND_MESSAGE_FRIEND_NOT_FOUND:
        return "The friend number did not designate a valid friend.";
    case TOX_ERR_FRIEND_SEND_MESSAGE_FRIEND_NOT_CONNECTED:
        return "This client is currently not connected to the friend.";
    case TOX_ERR_FRIEND_SEND_MESSAGE_SENDQ:
        return "An allocation error occurred while increasing the send queue size.";
    case TOX_ERR_FRIEND_SEND_MESSAGE_TOO_LONG:
        return "Message length exceeded TOX_MAX_MESSAGE_LENGTH.";
    case TOX_ERR_FRIEND_SEND_MESSAGE_EMPTY:
        return "Attempted to send a zero-length message.";
    default:
        return "Unknown error";
    }
}

std::string pTox::toxav_new_error(TOXAV_ERR_NEW error)
{
    switch (error) {
    case TOXAV_ERR_NEW_OK:
        return "The function returned successfully.";
    case TOXAV_ERR_NEW_NULL:
        return "One of the arguments to the function was NULL when it was not expected.";
    case TOXAV_ERR_NEW_MALLOC:
        return "Memory allocation failure while trying to allocate structures required for the A/V session.";
    case TOXAV_ERR_NEW_MULTIPLE:
        return "Attempted to create a second session for the same Tox instance.";
    default:
        return "Unknown error";
    }
}

std::string pTox::toxav_call_error(TOXAV_ERR_CALL_CONTROL error)
{
    switch (error) {
    case TOXAV_ERR_CALL_CONTROL_OK:
        return "The function returned successfully.";
    case TOXAV_ERR_CALL_CONTROL_SYNC:
        return "Synchronization error occurred.";
    case TOXAV_ERR_CALL_CONTROL_FRIEND_NOT_FOUND:
        return "The friend_number passed did not designate a valid friend.";
    case TOXAV_ERR_CALL_CONTROL_FRIEND_NOT_IN_CALL:
        return "This client is currently not in a call with the friend. Before the call is answered, only CANCEL is a valid control.";
    case TOXAV_ERR_CALL_CONTROL_INVALID_TRANSITION:
        return "Happens if user tried to pause an already paused call or if trying to resume a call that is not paused.";
    default:
        return "Unknown error";
    }
}

std::string pTox::toxav_answer_error(TOXAV_ERR_ANSWER error)
{
    switch (error) {
    case TOXAV_ERR_ANSWER_OK:
        return "The function returned successfully.";
    case TOXAV_ERR_ANSWER_SYNC:
        return "Synchronization error occurred.";
    case TOXAV_ERR_ANSWER_CODEC_INITIALIZATION:
        return "Failed to initialize codecs for call session. Note that codec initiation will fail if there is no receive callback registered for either audio or video.";
    case TOXAV_ERR_ANSWER_FRIEND_NOT_FOUND:
        return "The friend number did not designate a valid friend.";
    case TOXAV_ERR_ANSWER_FRIEND_NOT_CALLING:
        return "The friend was valid, but they are not currently trying to initiate a call. This is also returned if this client is already in a call with the friend.";
    case TOXAV_ERR_ANSWER_INVALID_BIT_RATE:
        return "Audio or video bit rate is invalid.";
    default:
        return "Unknown error";
    }
}

std::string pTox::toxav_send_frame_error(TOXAV_ERR_SEND_FRAME error)
{
    switch (error) {
    case TOXAV_ERR_SEND_FRAME_OK:
        return "The function returned successfully.";
    case TOXAV_ERR_SEND_FRAME_NULL:
        return "In case of video, one of Y, U, or V was NULL. In case of audio, the samples data pointer was NULL.";
    case TOXAV_ERR_SEND_FRAME_FRIEND_NOT_FOUND:
        return "The friend_number passed did not designate a valid friend.";
    case TOXAV_ERR_SEND_FRAME_FRIEND_NOT_IN_CALL:
        return "This client is currently not in a call with the friend.";
    case TOXAV_ERR_SEND_FRAME_SYNC:
        return "Synchronization error occurred.";
    case TOXAV_ERR_SEND_FRAME_INVALID:
        return "One of the frame parameters was invalid. E.g. the resolution may be too small or too large, or the audio sampling rate may be unsupported.";
    case TOXAV_ERR_SEND_FRAME_PAYLOAD_TYPE_DISABLED:
        return "Either friend turned off audio or video receiving or we turned off sending for the said payload.";
    case TOXAV_ERR_SEND_FRAME_RTP_FAILED:
        return "Failed to push frame through rtp interface.";
    default:
        return "Unknown error";
    }
}
