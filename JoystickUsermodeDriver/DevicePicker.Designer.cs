namespace JoystickUsermodeDriver
{
  partial class DevicePicker
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
      this.label1 = new System.Windows.Forms.Label();
      this.label2 = new System.Windows.Forms.Label();
      this.m_joystickDevice = new System.Windows.Forms.ListBox();
      this.m_rudderDevice = new System.Windows.Forms.ListBox();
      this.cancelButton = new System.Windows.Forms.Button();
      this.okButton = new System.Windows.Forms.Button();
      this.SuspendLayout();
      // 
      // label1
      // 
      this.label1.AutoSize = true;
      this.label1.Location = new System.Drawing.Point(12, 9);
      this.label1.Name = "label1";
      this.label1.Size = new System.Drawing.Size(82, 13);
      this.label1.TabIndex = 0;
      this.label1.Text = "Joystick Device";
      // 
      // label2
      // 
      this.label2.AutoSize = true;
      this.label2.Location = new System.Drawing.Point(241, 9);
      this.label2.Name = "label2";
      this.label2.Size = new System.Drawing.Size(77, 13);
      this.label2.TabIndex = 1;
      this.label2.Text = "Rudder Pedals";
      // 
      // m_joystickDevice
      // 
      this.m_joystickDevice.FormattingEnabled = true;
      this.m_joystickDevice.Location = new System.Drawing.Point(15, 25);
      this.m_joystickDevice.Name = "m_joystickDevice";
      this.m_joystickDevice.Size = new System.Drawing.Size(218, 121);
      this.m_joystickDevice.TabIndex = 2;
      // 
      // m_rudderDevice
      // 
      this.m_rudderDevice.FormattingEnabled = true;
      this.m_rudderDevice.Location = new System.Drawing.Point(244, 25);
      this.m_rudderDevice.Name = "m_rudderDevice";
      this.m_rudderDevice.Size = new System.Drawing.Size(218, 121);
      this.m_rudderDevice.TabIndex = 3;
      // 
      // cancelButton
      // 
      this.cancelButton.Location = new System.Drawing.Point(263, 158);
      this.cancelButton.Name = "cancelButton";
      this.cancelButton.Size = new System.Drawing.Size(75, 23);
      this.cancelButton.TabIndex = 4;
      this.cancelButton.Text = "Cancel";
      this.cancelButton.UseVisualStyleBackColor = true;
      this.cancelButton.Click += new System.EventHandler(this.cancelButton_Click);
      // 
      // okButton
      // 
      this.okButton.Location = new System.Drawing.Point(133, 158);
      this.okButton.Name = "okButton";
      this.okButton.Size = new System.Drawing.Size(75, 23);
      this.okButton.TabIndex = 5;
      this.okButton.Text = "OK";
      this.okButton.UseVisualStyleBackColor = true;
      this.okButton.Click += new System.EventHandler(this.okButton_Click);
      // 
      // DevicePicker
      // 
      this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
      this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
      this.ClientSize = new System.Drawing.Size(471, 190);
      this.Controls.Add(this.okButton);
      this.Controls.Add(this.cancelButton);
      this.Controls.Add(this.m_rudderDevice);
      this.Controls.Add(this.m_joystickDevice);
      this.Controls.Add(this.label2);
      this.Controls.Add(this.label1);
      this.Name = "DevicePicker";
      this.Text = "DevicePicker";
      this.ResumeLayout(false);
      this.PerformLayout();

    }

    #endregion

    private System.Windows.Forms.Label label1;
    private System.Windows.Forms.Label label2;
    private System.Windows.Forms.ListBox m_joystickDevice;
    private System.Windows.Forms.ListBox m_rudderDevice;
    private System.Windows.Forms.Button cancelButton;
    private System.Windows.Forms.Button okButton;
  }
}