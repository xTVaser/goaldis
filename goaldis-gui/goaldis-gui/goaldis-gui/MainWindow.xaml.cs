using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;

namespace goaldis_gui {

    /// <summary>
    /// Interaction logic for MainWindow.xaml
    /// </summary>
    public partial class MainWindow : Window {

        bool ps3 = false;
        bool individualFile = false;
        List<String> filePaths = new List<String>();
        String currentDir = "";
        String outputDir = "";

        public MainWindow() {
            InitializeComponent();
        }

        private void Window_Loaded(object sender, RoutedEventArgs e) {

        }

        private void ps2Radio_Checked(object sender, RoutedEventArgs e) {

            ps3 = false;
            submitBtn.IsEnabled = false;
            currentDir = "";
            filePaths = new List<String>();
            infoMsg.Text = "Current Directory: " + currentDir + "\n";
            checkReady();
        }

        private void ps3Radio_Checked(object sender, RoutedEventArgs e) {

            ps3 = true;
            fileButton.IsEnabled = true;
            submitBtn.IsEnabled = false;
            currentDir = "";
            filePaths = new List<String>();
            infoMsg.Text = "Current Directory: " + currentDir + "\n";
            checkReady();
        }

        private void fileButton_Click(object sender, RoutedEventArgs e) {

            String result = OpenFileDialog();
            if (result == null)
                return;

            individualFile = true;
            currentDir = result;
            infoMsg.Text = "Current File: " + currentDir + "\n";
            filePaths = new List<String>();
            filePaths.Add(result);
            checkReady();
        }

        private void directoryButton_Click(object sender, RoutedEventArgs e) {

            String result = OpenFolderDialog();
            if (result == null)
                return;

            individualFile = false;
            currentDir = result;
            infoMsg.Text = "Current Directory: " + currentDir + "\n";

            // Search the directory for relevant files and add to list.
            filePaths = new List<String>();
            String[] tempList = Directory.GetFiles(currentDir, "*.*", SearchOption.AllDirectories);
            String[] validExtensions = { ".cgo", ".dgo" };
            if (ps3)
                validExtensions = new String[] { ".go", ".o" };

            foreach (String file in tempList) {
                
                String extension = System.IO.Path.GetExtension(file).ToLower();

                if (validExtensions.Any(s => extension.Equals(s)))
                    filePaths.Add(file);
            }

            infoMsg.Text += "Found " + filePaths.Count + " files to disassemble.\n";

            checkReady();
        }

        private void outputButton_Click(object sender, RoutedEventArgs e) {

            String result = OpenFolderDialog();
            if (result == null)
                return;

            outputDir = result;
            infoMsg.Text += "Output Directory: " + outputDir + "\n";
            checkReady();
        }

        // Ensure that everything is ready before running the disassembler
        private void checkReady() {

            if (currentDir.Equals("") is false && 
                outputDir.Equals("") is false && 
                filePaths != null && 
                ((bool)ps2Radio.IsChecked || (bool)ps3Radio.IsChecked)) {

                submitBtn.IsEnabled = true;
            }
        }

        private void submitBtn_Click(object sender, RoutedEventArgs e) {
            
            foreach (String f in filePaths) {
                
                String dir = outputDir;
                // for ps3 files, they are nested inside a final/game folder within the more general category
                if (ps3 && individualFile is false)
                    dir += "\\" + Directory.GetParent(f).Parent.Name.ToString();
                // Otherwise its ps2 and a container file
                else if (ps3 is false)
                    dir += "\\" + System.IO.Path.GetFileNameWithoutExtension(f).ToString();
                
                // Spawn a thread for each exe call for speed and GUI updating purposes
                Thread t = new Thread(() => runDisassembler(f, dir));
                t.IsBackground = true;
                t.Start();
            }
        }

        private void runDisassembler(String file, String dir) {

            this.Dispatcher.Invoke(() => { logBox.Text += "Started Disassembling " + file + "\n"; });
            
            // TODO rich textbox for some color, and horizontal scrollbar or wrap it
            ProcessStartInfo callExe = new ProcessStartInfo();
            callExe.CreateNoWindow = true;
            callExe.UseShellExecute = true;
            callExe.WorkingDirectory = System.IO.Directory.GetCurrentDirectory();
            callExe.FileName = "goaldis.exe";
            callExe.WindowStyle = ProcessWindowStyle.Hidden;
            if (ps3)
                callExe.Arguments = "-file \"" + dir + "\" \"" + file + "\"";
            else
                callExe.Arguments = "-asm \"" + dir + "\" \"" + file + "\"";

            try {
                using (Process exeProcess = Process.Start(callExe)) {
                    exeProcess.WaitForExit();
                }
            }
            catch {
            }

            this.Dispatcher.Invoke(() => { logBox.Text += "Finished Disassembling " + file + ", stored at " + dir + "\n"; });
        }

        private String OpenFileDialog() {

            Microsoft.Win32.OpenFileDialog fileDialog = new Microsoft.Win32.OpenFileDialog();
            if (ps3 is false)
                fileDialog.Filter = "Container Files (*.DGO, *.CGO)|*.DGO;*.CGO";
            else
                fileDialog.Filter = "Individual Files (*.go, *.o)|*.go;*.o";

            bool? result = fileDialog.ShowDialog();

            if (result is true)
                return fileDialog.FileName;

            return null;
        }

        private String OpenFolderDialog() {

            System.Windows.Forms.FolderBrowserDialog folderDialog = new System.Windows.Forms.FolderBrowserDialog();

            System.Windows.Forms.DialogResult result = folderDialog.ShowDialog();

            if (result == System.Windows.Forms.DialogResult.OK)
                return folderDialog.SelectedPath;

            return null;
        }
    }
}
