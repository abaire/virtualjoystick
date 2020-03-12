using System.Windows.Forms;

namespace JoystickUsermodeDriver
{
    partial class MainFrame
    {
        /// <summary>
        /// Required designer variable.
        /// </summary>
        private System.ComponentModel.IContainer components = null;

        /// <summary>
        /// Clean up any resources being used.
        /// </summary>
        /// <param name="disposing">true if managed resources should be disposed; otherwise, false.</param>
        protected override void Dispose(bool disposing)
        {
            if (disposing && (components != null))
            {
                components.Dispose();
            }
            base.Dispose(disposing);
        }

        #region Windows Form Designer generated code

        /// <summary>
        /// Required method for Designer support - do not modify
        /// the contents of this method with the code editor.
        /// </summary>
        private void InitializeComponent()
        {
            this.components = new System.ComponentModel.Container();
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(MainFrame));
            this._systemTrayIcon = new System.Windows.Forms.NotifyIcon(this.components);
            this._contextMenuStrip = new System.Windows.Forms.ContextMenuStrip(this.components);
            this.MenuShow = new System.Windows.Forms.ToolStripMenuItem();
            this.reloadActiveProfileToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.MenuClose = new System.Windows.Forms.ToolStripMenuItem();
            this._mainMenuStrip = new System.Windows.Forms.MenuStrip();
            this.hideToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.refreshToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.reloadActiveProfileToolStripMenuItem1 = new System.Windows.Forms.ToolStripMenuItem();
            this.profileList = new System.Windows.Forms.ListBox();
            this.label1 = new System.Windows.Forms.Label();
            this._profileEditor = new JoystickUsermodeDriver.ProfileEditorPanel();
            this._contextMenuStrip.SuspendLayout();
            this._mainMenuStrip.SuspendLayout();
            this.SuspendLayout();
            // 
            // _systemTrayIcon
            // 
            this._systemTrayIcon.BalloonTipText = "Virtual Joystick Driver";
            this._systemTrayIcon.ContextMenuStrip = this._contextMenuStrip;
            this._systemTrayIcon.Icon = ((System.Drawing.Icon)(resources.GetObject("_systemTrayIcon.Icon")));
            this._systemTrayIcon.Text = "Virtual Joystick Driver";
            this._systemTrayIcon.Visible = true;
            // 
            // _contextMenuStrip
            // 
            this._contextMenuStrip.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.MenuShow,
            this.reloadActiveProfileToolStripMenuItem,
            this.MenuClose});
            this._contextMenuStrip.Name = "contextMenuStrip";
            this._contextMenuStrip.Size = new System.Drawing.Size(182, 70);
            // 
            // MenuShow
            // 
            this.MenuShow.Name = "MenuShow";
            this.MenuShow.Size = new System.Drawing.Size(181, 22);
            this.MenuShow.Text = "Show";
            this.MenuShow.Click += new System.EventHandler(this.MenuShow_Click);
            // 
            // reloadActiveProfileToolStripMenuItem
            // 
            this.reloadActiveProfileToolStripMenuItem.Name = "reloadActiveProfileToolStripMenuItem";
            this.reloadActiveProfileToolStripMenuItem.Size = new System.Drawing.Size(181, 22);
            this.reloadActiveProfileToolStripMenuItem.Text = "Reload active profile";
            this.reloadActiveProfileToolStripMenuItem.Click += new System.EventHandler(this.reloadActiveProfileToolStripMenuItem_Click);
            // 
            // MenuClose
            // 
            this.MenuClose.Name = "MenuClose";
            this.MenuClose.Size = new System.Drawing.Size(181, 22);
            this.MenuClose.Text = "Close";
            this.MenuClose.Click += new System.EventHandler(this.MenuClose_Click);
            // 
            // _mainMenuStrip
            // 
            this._mainMenuStrip.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.hideToolStripMenuItem,
            this.refreshToolStripMenuItem,
            this.reloadActiveProfileToolStripMenuItem1});
            this._mainMenuStrip.Location = new System.Drawing.Point(5, 5);
            this._mainMenuStrip.Name = "_mainMenuStrip";
            this._mainMenuStrip.Size = new System.Drawing.Size(799, 24);
            this._mainMenuStrip.TabIndex = 34;
            this._mainMenuStrip.Text = "mainMenuStrip";
            // 
            // hideToolStripMenuItem
            // 
            this.hideToolStripMenuItem.Name = "hideToolStripMenuItem";
            this.hideToolStripMenuItem.Size = new System.Drawing.Size(44, 20);
            this.hideToolStripMenuItem.Text = "Hide";
            this.hideToolStripMenuItem.Click += new System.EventHandler(this.hideToolStripMenuItem_Click);
            // 
            // refreshToolStripMenuItem
            // 
            this.refreshToolStripMenuItem.Name = "refreshToolStripMenuItem";
            this.refreshToolStripMenuItem.Size = new System.Drawing.Size(58, 20);
            this.refreshToolStripMenuItem.Text = "Refresh";
            this.refreshToolStripMenuItem.Click += new System.EventHandler(this.refreshToolStripMenuItem_Click);
            // 
            // reloadActiveProfileToolStripMenuItem1
            // 
            this.reloadActiveProfileToolStripMenuItem1.Name = "reloadActiveProfileToolStripMenuItem1";
            this.reloadActiveProfileToolStripMenuItem1.Size = new System.Drawing.Size(128, 20);
            this.reloadActiveProfileToolStripMenuItem1.Text = "Reload Active Profile";
            this.reloadActiveProfileToolStripMenuItem1.Click += new System.EventHandler(this.reloadActiveProfileToolStripMenuItem_Click);
            // 
            // profileList
            // 
            this.profileList.FormattingEnabled = true;
            this.profileList.Location = new System.Drawing.Point(13, 60);
            this.profileList.Name = "profileList";
            this.profileList.Size = new System.Drawing.Size(227, 251);
            this.profileList.TabIndex = 36;
            this.profileList.SelectedIndexChanged += new System.EventHandler(this.profileList_SelectedIndexChanged);
            // 
            // label1
            // 
            this.label1.AutoSize = true;
            this.label1.Location = new System.Drawing.Point(17, 44);
            this.label1.Name = "label1";
            this.label1.Size = new System.Drawing.Size(36, 13);
            this.label1.TabIndex = 37;
            this.label1.Text = "Profile";
            // 
            // _profileEditor
            // 
            this._profileEditor.AutoScroll = true;
            this._profileEditor.AutoScrollMinSize = new System.Drawing.Size(20, 0);
            this._profileEditor.BackColor = System.Drawing.SystemColors.Window;
            this._profileEditor.BorderStyle = System.Windows.Forms.BorderStyle.FixedSingle;
            this._profileEditor.ColumnCount = 3;
            this._profileEditor.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle(System.Windows.Forms.SizeType.Percent, 6F));
            this._profileEditor.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle(System.Windows.Forms.SizeType.Percent, 33F));
            this._profileEditor.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle(System.Windows.Forms.SizeType.Percent, 35F));
            this._profileEditor.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle(System.Windows.Forms.SizeType.Percent, 8.108109F));
            this._profileEditor.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle(System.Windows.Forms.SizeType.Percent, 44.59459F));
            this._profileEditor.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle(System.Windows.Forms.SizeType.Percent, 47.2973F));
            this._profileEditor.Location = new System.Drawing.Point(260, 39);
            this._profileEditor.Name = "_profileEditor";
            this._profileEditor.RowCount = 1;
            this._profileEditor.RowStyles.Add(new System.Windows.Forms.RowStyle(System.Windows.Forms.SizeType.Absolute, 270F));
            this._profileEditor.RowStyles.Add(new System.Windows.Forms.RowStyle(System.Windows.Forms.SizeType.Absolute, 270F));
            this._profileEditor.Size = new System.Drawing.Size(537, 272);
            this._profileEditor.TabIndex = 38;
            // 
            // MainFrame
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(809, 354);
            this.ControlBox = false;
            this.Controls.Add(this._profileEditor);
            this.Controls.Add(this.label1);
            this.Controls.Add(this.profileList);
            this.Controls.Add(this._mainMenuStrip);
            this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedDialog;
            this.MainMenuStrip = this._mainMenuStrip;
            this.MaximumSize = new System.Drawing.Size(825, 393);
            this.MinimumSize = new System.Drawing.Size(825, 393);
            this.Name = "MainFrame";
            this.Padding = new System.Windows.Forms.Padding(5);
            this.ShowIcon = false;
            this.ShowInTaskbar = false;
            this.Text = "Virtual Joystick Driver";
            this.WindowState = System.Windows.Forms.FormWindowState.Minimized;
            this.FormClosed += new System.Windows.Forms.FormClosedEventHandler(this.MainFrame_FormClosed);
            this._contextMenuStrip.ResumeLayout(false);
            this._mainMenuStrip.ResumeLayout(false);
            this._mainMenuStrip.PerformLayout();
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion
      private System.Windows.Forms.NotifyIcon _systemTrayIcon;
      private System.Windows.Forms.MenuStrip _mainMenuStrip;
        private System.Windows.Forms.ContextMenuStrip _contextMenuStrip;
        private System.Windows.Forms.ToolStripMenuItem MenuShow;
        private System.Windows.Forms.ToolStripMenuItem MenuClose;
        private ToolStripMenuItem reloadActiveProfileToolStripMenuItem;
        private ToolStripMenuItem refreshToolStripMenuItem;
        private ToolStripMenuItem reloadActiveProfileToolStripMenuItem1;
        private ListBox profileList;
        private Label label1;
        private ProfileEditorPanel _profileEditor;
        private ToolStripMenuItem hideToolStripMenuItem;
    }
}

