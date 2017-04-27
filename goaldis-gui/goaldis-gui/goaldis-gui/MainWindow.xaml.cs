using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
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
            fileButton.IsEnabled = false;
            submitBtn.IsEnabled = false;
            currentDir = "";
            infoMsg.Text = "Current Directory: " + currentDir + "\n";
            checkReady();
        }

        private void ps3Radio_Checked(object sender, RoutedEventArgs e) {

            ps3 = true;
            fileButton.IsEnabled = true;
            submitBtn.IsEnabled = false;
            currentDir = "";
            infoMsg.Text = "Current Directory: " + currentDir + "\n";
            checkReady();
        }

        private void fileButton_Click(object sender, RoutedEventArgs e) {

            String result = OpenFileDialog();
            if (result == null)
                return;

            currentDir = result;
            infoMsg.Text = "Current File: " + currentDir + "\n";
            checkReady();
        }

        private void directoryButton_Click(object sender, RoutedEventArgs e) {

            String result = OpenFolderDialog();
            if (result == null)
                return;

            currentDir = result;
            infoMsg.Text = "Current Directory: " + currentDir + "\n";

            // Search the directory for relevant files and add to list.
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
            infoMsg.Text = "Current Directory: " + currentDir + "\n";
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
