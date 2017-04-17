#ifndef BITCOIN_IPC_INTERFACES_H
#define BITCOIN_IPC_INTERFACES_H

#include <memory>
#include <string>

namespace ipc {

class Handler;

//! Top-level interface for a bitcoin node (bitcoind process).
class Node
{
public:
    virtual ~Node() {}

    //! Set command line arguments.
    virtual void parseParameters(int argc, const char* const argv[]) = 0;

    //! Load settings from configuration file.
    virtual void readConfigFile(const std::string& conf_path) = 0;

    //! Choose network parameters.
    virtual void selectParams(const std::string& network) = 0;

    //! Init logging.
    virtual void initLogging() = 0;

    //! Init parameter interaction.
    virtual void initParameterInteraction() = 0;

    //! Get warnings.
    virtual std::string getWarnings(const std::string& type) = 0;

    //! Start node.
    virtual bool appInit() = 0;

    //! Stop node.
    virtual void appShutdown() = 0;

    //! Start shutdown.
    virtual void startShutdown() = 0;

    //! Register handler for init messages.
    using InitMessageFn = std::function<void(const std::string& message)>;
    virtual std::unique_ptr<Handler> handleInitMessage(InitMessageFn fn) = 0;
};

//! Interface for managing a registered handler.
class Handler
{
public:
    virtual ~Handler() {}

    //! Disconnect the handler.
    virtual void disconnect() = 0;
};

//! Protocol IPC interface should use to communicate with implementation.
enum Protocol {
    LOCAL, //!< Call functions linked into current executable.
};

//! Create IPC node interface, communicating with requested protocol. Returns
//! null if protocol isn't implemented or is not available in the current build
//! configuation.
std::unique_ptr<Node> MakeNode(Protocol protocol);

} // namespace ipc

#endif // BITCOIN_IPC_INTERFACES_H
