using System;
using System.Collections.Generic;
using System.Windows.Forms;

namespace JoystickUsermodeDriver
{
    static class Program
    {
        /// <summary>
        /// The main entry point for the application.
        /// </summary>
        [STAThread]
        static void Main()
        {
            var entryAssembly = System.Reflection.Assembly.GetEntryAssembly().Location;
            var processName = System.IO.Path.GetFileNameWithoutExtension(entryAssembly);

            // Prevent duplicate instances.
            if (System.Diagnostics.Process.GetProcessesByName(processName).Length > 1)
            {
                MessageBox.Show("Application is already running, exiting.",
                    "Already running",
                    MessageBoxButtons.OK,
                    MessageBoxIcon.Exclamation);
                return;
            }

            Application.EnableVisualStyles();
            Application.SetCompatibleTextRenderingDefault(false);
            Application.Run(new MainFrame());
        }
    }
}