/* Copyright 2025 Marc Scheffer
 *
 * helmBoy is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 *
 * This work is based on bepzi's Helm project, <https://github.com/bepzi/helm>,
 * itself based on Matt Tytel's Helm <https://tytel.org/helm/>
 *
 * helmBoy is distributedin the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with helmBoy.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <JuceHeader.h>
#include <exception>
#include <csignal>
#include <cstdlib>
#if JUCE_WINDOWS
  #ifndef NOMINMAX
    #define NOMINMAX 1
  #endif
  #ifndef WIN32_LEAN_AND_MEAN
    #define WIN32_LEAN_AND_MEAN 1
  #endif
  #ifndef NOGDI
    #define NOGDI 1
  #endif
  #include <windows.h>
  #include <dbghelp.h>
  #include <eh.h>
  #ifdef Rectangle
    #undef Rectangle
  #endif
  #ifdef min
    #undef min
  #endif
  #ifdef max
    #undef max
  #endif
#endif
#include "border_bounds_constrainer.h"
#include "helmBoy_editor.h"
#include "load_save.h"
#include "startup_trace.h"

namespace {
  #define appendStartupTrace(message) HELMBOY_STARTUP_TRACE("startup_trace.log", message)

  void showCriticalMessage(const String& message) {
    SystemClipboard::copyTextToClipboard(message);

    auto displayMessage = message + "\n\n(Le texte a ete copie dans le presse-papiers.)";
    MessageManager::callAsync([displayMessage]() {
      AlertWindow::showMessageBoxAsync(AlertWindow::WarningIcon,
                                       "helmBoy - Message critique",
                                       displayMessage,
                                       "OK");
    });
  }

  void logHardCrash(const String& message) {
    showCriticalMessage(message);
#if JUCE_WINDOWS
    OutputDebugStringW(message.toWideCharPointer());
    OutputDebugStringW(L"\n");
#endif
  }

#if JUCE_WINDOWS
  bool writeWindowsMinidump(EXCEPTION_POINTERS* exception_info, String& out_path) {
    auto logs_dir = File::getSpecialLocation(File::userApplicationDataDirectory)
                      .getChildFile("helmBoy")
                      .getChildFile("logs");
    (void)logs_dir.createDirectory();

    auto dump_name = "crash_" + Time::getCurrentTime().formatted("%Y%m%d_%H%M%S_%3Q") + ".dmp";
    auto dump_file = logs_dir.getChildFile(dump_name);
    out_path = dump_file.getFullPathName();

    HANDLE file = CreateFileW(out_path.toWideCharPointer(),
                              GENERIC_WRITE,
                              0,
                              nullptr,
                              CREATE_ALWAYS,
                              FILE_ATTRIBUTE_NORMAL,
                              nullptr);
    if (file == INVALID_HANDLE_VALUE)
      return false;

    MINIDUMP_EXCEPTION_INFORMATION ex_info{};
    ex_info.ThreadId = GetCurrentThreadId();
    ex_info.ExceptionPointers = exception_info;
    ex_info.ClientPointers = FALSE;

    const auto dump_type = static_cast<MINIDUMP_TYPE>(
      MiniDumpWithThreadInfo | MiniDumpWithIndirectlyReferencedMemory);

    BOOL ok = MiniDumpWriteDump(GetCurrentProcess(),
                                GetCurrentProcessId(),
                                file,
                                dump_type,
                                exception_info != nullptr ? &ex_info : nullptr,
                                nullptr,
                                nullptr);
    CloseHandle(file);
    return ok == TRUE;
  }

  LONG WINAPI helmBoyUnhandledExceptionFilter(EXCEPTION_POINTERS* exception_info) {
    String dump_path;
    bool dump_ok = writeWindowsMinidump(exception_info, dump_path);

    if (exception_info != nullptr && exception_info->ExceptionRecord != nullptr) {
      auto* rec = exception_info->ExceptionRecord;
      String code = String::toHexString(static_cast<int64>(static_cast<uint32>(rec->ExceptionCode))).toUpperCase();
      String addr = String::toHexString(reinterpret_cast<uint64>(rec->ExceptionAddress)).toUpperCase();
      logHardCrash("[Crash] Unhandled SEH exception code=0x" + code +
                   " address=0x" + addr +
                   " | dump=" + (dump_ok ? dump_path : String("<failed>")));
    }
    else {
      logHardCrash("[Crash] Unhandled SEH exception with null EXCEPTION_POINTERS. | dump=" +
                   (dump_ok ? dump_path : String("<failed>")));
    }

    return EXCEPTION_CONTINUE_SEARCH;
  }

  void __cdecl helmBoyPurecallHandler() {
    logHardCrash("[Crash] _purecall invoked (virtual call during invalid object lifetime).");
  }
#endif

  void helmBoyTerminateHandler() {
    auto eptr = std::current_exception();
    if (eptr != nullptr) {
      try {
        std::rethrow_exception(eptr);
      }
      catch (const std::exception& ex) {
        logHardCrash("[Crash] std::terminate after exception: " + String(ex.what()));
      }
      catch (...) {
        logHardCrash("[Crash] std::terminate after unknown exception.");
      }
    }
    else {
      logHardCrash("[Crash] std::terminate called without active exception.");
    }

    std::abort();
  }

  void helmBoySignalHandler(int sig) {
    logHardCrash("[Crash] C runtime signal received: " + String(sig));
    std::abort();
  }

  void installHardCrashHandlers() {
#if JUCE_WINDOWS
    SetUnhandledExceptionFilter(helmBoyUnhandledExceptionFilter);
    _set_purecall_handler(helmBoyPurecallHandler);
#endif
    std::set_terminate(helmBoyTerminateHandler);
    std::signal(SIGABRT, helmBoySignalHandler);
    std::signal(SIGSEGV, helmBoySignalHandler);
    std::signal(SIGILL, helmBoySignalHandler);
    std::signal(SIGFPE, helmBoySignalHandler);
  }
}

class HelmBoyApplication : public JUCEApplication {
  public:
    class MainWindow : public DocumentWindow,
                       public ApplicationCommandTarget,
                       private AsyncUpdater {
    public:
      enum PatchCommands {
        kSave = 0x5001,
        kSaveAs,
        kOpen,
      };

      MainWindow(String name, bool visible = true) :
          DocumentWindow(name, Colours::lightgrey, DocumentWindow::allButtons, visible) {
        visible_ = visible;
        if (visible) {

          setUsingNativeTitleBar(true);
          setResizable(true, true);

          constrainer_.setMinimumSize(2 * mopo::DEFAULT_WINDOW_WIDTH / 3,
                                      2 * mopo::DEFAULT_WINDOW_HEIGHT / 3);
          if (auto* peer = getPeer()) {
            if (const auto frame_size = peer->getFrameSizeIfPresent())
              constrainer_.setBorder(*frame_size);
            else
              constrainer_.setBorder(BorderSize<int>(8));
          }
          else
            constrainer_.setBorder(BorderSize<int>(8));

          double ratio = (1.0 * mopo::DEFAULT_WINDOW_WIDTH) / mopo::DEFAULT_WINDOW_HEIGHT;

          constrainer_.setFixedAspectRatio(ratio);
          setConstrainer(&constrainer_);

          setSize(mopo::DEFAULT_WINDOW_WIDTH, mopo::DEFAULT_WINDOW_HEIGHT);
          centreWithSize(mopo::DEFAULT_WINDOW_WIDTH, mopo::DEFAULT_WINDOW_HEIGHT);

          if (auto* peer = getPeer()) {
            if (const auto frame_size = peer->getFrameSizeIfPresent())
              constrainer_.setBorder(*frame_size);
          }

          createEditor();
        }
        else {
          createEditor();
        }
      }

      void createEditor() {
        appendStartupTrace("createEditor start");

        if (editor_ != nullptr)
          return;

        try {
          if (visible_ && getPeer() == nullptr) {
            setVisible(true);
            toFront(true);

            if (getPeer() == nullptr) {
              reportStartupFailure("[Standalone Startup] Le peer de fenetre n'est pas pret pour creer l'editeur.");
              return;
            }
          }

          editor_ = new HelmBoyEditor(false);

          if (visible_) {
            if (!editor_->initializeGuiComponents()) {
              delete editor_;
              editor_ = nullptr;
              reportStartupFailure("[Standalone Startup] L'initialisation GUI a echoue.");
              return;
            }

            editor_->animate(LoadSave::shouldAnimateWidgets());
            editor_->ensureGuiReady();

            setContentOwned(editor_, true);

            setVisible(true);
            toFront(true);

            if (auto* peer = getPeer()) {
              if (const auto frame_size = peer->getFrameSizeIfPresent())
                constrainer_.setBorder(*frame_size);
            }

            triggerAsyncUpdate();
          }
          else {
            editor_->animate(false);
          }

          startAudio();
          appendStartupTrace("createEditor success");
        }
        catch (const std::exception& ex) {
          appendStartupTrace("createEditor exception: " + String(ex.what()));
          if (editor_ != nullptr) {
            if (getContentComponent() == editor_)
              clearContentComponent();
            else
              delete editor_;
            editor_ = nullptr;
          }

          reportStartupFailure("[Standalone Startup] Exception pendant la creation de l'editeur: " + String(ex.what()));
        }
        catch (...) {
          appendStartupTrace("createEditor unknown exception");
          if (editor_ != nullptr) {
            if (getContentComponent() == editor_)
              clearContentComponent();
            else
              delete editor_;
            editor_ = nullptr;
          }

          reportStartupFailure("[Standalone Startup] Exception inconnue pendant la creation de l'editeur.");
        }
      }

      void closeButtonPressed() override {
        JUCEApplication::getInstance()->systemRequestedQuit();
      }

      bool loadFile(File file) {
        if (editor_ == nullptr)
          return false;
        bool success = editor_->loadFromFile(file);
        if (success)
          editor_->externalPatchLoaded(file);
        return success;
      }

      ApplicationCommandTarget* getNextCommandTarget() override {
        return findFirstTargetParentComponent();
      }

      void getAllCommands(Array<CommandID>& commands) override {
        commands.add(kSave);
        commands.add(kSaveAs);
        commands.add(kOpen);
      }

      void getCommandInfo(const CommandID commandID, ApplicationCommandInfo& result) override {
        if (commandID == kSave) {
          result.setInfo(TRANS("Save"), TRANS("Saves the current patch"), "Application", 0);
          result.defaultKeypresses.add(KeyPress('s', ModifierKeys::commandModifier, 0));
        }
        else if (commandID == kSaveAs) {
          result.setInfo(TRANS("Save As"), TRANS("Saves patch to a new file"), "Application", 0);
          ModifierKeys modifier = ModifierKeys::commandModifier | ModifierKeys::shiftModifier;
          result.defaultKeypresses.add(KeyPress('s', modifier, 0));
        }
        else if (commandID == kOpen) {
          result.setInfo(TRANS("Open"), TRANS("Opens a patch"), "Application", 0);
          result.defaultKeypresses.add(KeyPress('o', ModifierKeys::commandModifier, 0));
        }
      }

      void open() {
        if (editor_ == nullptr)
          return;

        open_chooser_ = std::make_unique<FileChooser>("Open Patch", File(),
                                                     LoadSave::getSupportedPatchFileWildcard());
        open_chooser_->launchAsync(FileBrowserComponent::openMode | FileBrowserComponent::canSelectFiles,
                           [safe_window = Component::SafePointer<MainWindow>(this)](const FileChooser& chooser) {
                             auto results = chooser.getResults();
                             if (safe_window != nullptr && results.size() > 0) {
                               safe_window->loadFile(results[0]);
                             }
                           });
      }

      bool perform(const InvocationInfo& info) override {
        if (editor_ == nullptr)
          return false;

        if (info.commandID == kSave) {
          if (!editor_->saveToActiveFile())
            (void)editor_->exportToFile();
          (void)grabKeyboardFocus();
          (void)editor_->setFocus();
          return true;
        }
        if (info.commandID == kSaveAs) {
          (void)editor_->exportToFile();
          (void)grabKeyboardFocus();
          (void)editor_->setFocus();
          return true;
        }
        if (info.commandID == kOpen) {
          open();
          (void)grabKeyboardFocus();
          (void)editor_->setFocus();
          return true;
        }

        return false;
      }

      void handleAsyncUpdate() override {
        command_manager_ = std::make_unique<ApplicationCommandManager>();
        command_manager_->registerAllCommandsForTarget(JUCEApplication::getInstance());
        command_manager_->registerAllCommandsForTarget(this);
        addKeyListener(command_manager_->getKeyMappings());
        if (editor_)
          editor_->setFocus();
      }

      void startAudio() {
        if (editor_ == nullptr)
          return;

        if (!editor_->startAudioSubsystem())
          reportStartupFailure("[Standalone Startup] Le sous-systeme audio n'a pas pu demarrer. Verifiez le peripherique audio puis relancez HelmBoy.");
      }

      void activeWindowStatusChanged() override {
        if (editor_)
          editor_->animate(LoadSave::shouldAnimateWidgets() && isShowing());

        if (editor_ && isActiveWindow())
          editor_->grabKeyboardFocus();
      }

    private:
      void reportStartupFailure(const String& message) {
        Logger::writeToLog(message);
        if (visible_)
          showCriticalMessage(message);
        else if (auto* app = JUCEApplication::getInstance())
          app->quit();
      }

      JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainWindow)
      std::unique_ptr<FileChooser> open_chooser_;
      HelmBoyEditor* editor_ = nullptr;
      bool visible_ = true;
      std::unique_ptr<ApplicationCommandManager> command_manager_;
      BorderBoundsConstrainer constrainer_;
    };

    HelmBoyApplication() = default;

    const String getApplicationName() override { return ProjectInfo::projectName; }
    const String getApplicationVersion() override { return ProjectInfo::versionString; }
    bool moreThanOneInstanceAllowed() override { return false; }

    void initialise(const String& command_line) override {
      installHardCrashHandlers();

      String command = " " + command_line + " ";
      if (command.contains(" --version ") || command.contains(" -v ")) {
        std::cout << getApplicationName() << " " << getApplicationVersion() << newLine;
        quit();
      }
      else if (command.contains(" --help ") || command.contains(" -h ")) {
        std::cout << "Usage:" << newLine;
        std::cout << "  " << getApplicationName().toLowerCase() << " [OPTION...]" << newLine << newLine;
        std::cout << getApplicationName() << " polyphonic, semi-modular synthesizer." << newLine << newLine;
        std::cout << "Help Options:" << newLine;
        std::cout << "  -h, --help                          Show help options" << newLine << newLine;
        std::cout << "Application Options:" << newLine;
        std::cout << "  -v, --version                       Show version information and exit" << newLine;
        std::cout << "  --headless                          Run without graphical interface." << newLine << newLine;
        quit();
      }
      else {
        bool visible = !command.contains(" --headless ");

        main_window_ = std::make_unique<MainWindow>(getApplicationName(), visible);

        StringArray args = getCommandLineParameterArray();
        File file;

        std::ranges::for_each(std::views::iota(0, args.size()), [&](int i) {
          if (args[i] != "" && args[i][0] != '-' && loadFromCommandLine(args[i]))
            return;
        });
      }
    }

    bool loadFromCommandLine(const String& command_line) {
      String file_path = command_line;
      if (file_path[0] == '"' && file_path[file_path.length() - 1] == '"')
        file_path = command_line.substring(1, command_line.length() - 1);
      File file = File::getCurrentWorkingDirectory().getChildFile(file_path);
      if (file.exists())
        return main_window_->loadFile(file);
      return false;
    }

    void shutdown() override {
      main_window_ = nullptr;
    }

    void systemRequestedQuit() override {
      quit();
    }

    void anotherInstanceStarted(const String& command_line) override {
      loadFromCommandLine(command_line);
    }

  private:
    std::unique_ptr<MainWindow> main_window_;
    };

  START_JUCE_APPLICATION(HelmBoyApplication)
