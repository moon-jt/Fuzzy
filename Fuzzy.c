/*版本控制：第一版v1.0            20200804  19:14*/
using GasHeaterWizard.SQLManager;
using System;
using System.Collections.Generic;
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
using Xceed.Wpf.Toolkit;

namespace GasHeaterWizard.pages
{
    /// <summary>
    /// fuzzy.xaml 的交互逻辑
    /// </summary>
    public partial class fuzzy : Page
    {
        private int combination_flag;
        private StackPanel stackPanelPoints;
        private readonly int POINT_NUM = 5;

        private ProjectRecord record;
        public fuzzy()
        {
            InitializeComponent();
            checkBoxs = new Dictionary<string, CheckBox>();/*定义checkBox集*/
            LoadRecord();
            CreateUI();

            //MainWindow.TimerTicked += MainWindow_TimerTicked;/*面板数据交互*/

            MainWindow.DataFrameReceived += MainWindow_DataFrameReceived;
            this.Unloaded += CurvePage_Unloaded;
        }
        /*以上为初始化过程*可将预处理命令放在上面功能块*/

        /* 页面卸载，取消注册事件 */
        private void CurvePage_Unloaded(object sender, RoutedEventArgs e)
        {
            //MainWindow.TimerTicked -= MainWindow_TimerTicked;
            MainWindow.DataFrameReceived -= MainWindow_DataFrameReceived;
            this.Unloaded -= CurvePage_Unloaded;
        }

        /* 接收到数据帧 */
        private int CurrentDuty;

        private void MainWindow_DataFrameReceived(DataRecord obj)
        {
            CurrentDuty = Convert.ToInt32((obj.TmpOut - obj.TmpIn) * obj.Flux / 25.0f);
            /*接收到的数据要需要保存到数据库 */
            MainWindow.allRunningDatas.Add(obj);
            SqlManager.InsertRecord2RunningDataTable(record, obj);
            /* 曲线窗口处于打开状态，更新曲线显示 */
            if (MainWindow.ChartWin != null)
            {
                MainWindow.ChartWin.FillDataSeries(obj);
            }
        }



        #region/*获取曲线数据*/
        private void LabelA_MouseDoubleClick(object sender, MouseButtonEventArgs e)
        {
            Label label = sender as Label;
            int index = Convert.ToInt32(label.Tag);/*区分分段状态1:0-1 2:1-0 3:1-2 4:2-1 5:2-3 6:3-2*/


            int len;
            int DataCount;
            DataCount = 50;                             /*读取数据数*/
            /* 读取曲线数据点 */
            if (MainWindow.ChartWin != null)/*曲线窗口打开*/
            {
                len = MainWindow.ChartWin.PointMarkersSelectionModifier.SelectedPointMarkers.Count;

                if (len < DataCount)
                {
                    System.Windows.MessageBox.Show("Need more Point!!");
                }
                else
                {
                    double rawXBuff = 0.0f;
                    len = 0;
                    foreach (var item in MainWindow.ChartWin.PointMarkersSelectionModifier.SelectedPointMarkers)
                    {
                        if (rawXBuff != Convert.ToDouble(item.XValue))  /*将存储的item数值转化为Double*/
                        {
                            len++;
                            rawXBuff = Convert.ToDouble(item.XValue);
                        }
                    }

                    if (len < DataCount)
                    {
                        System.Windows.MessageBox.Show("Need more Point!!");
                        return;
                    }

                    double[] x = new double[len];
                    double[] y = new double[len];

                    double[] ratio = new double[] { 0.0, 0.0 };/* 二元组，用来存放指数和系数 */

                    int i = 0;
                    double xv = 0.0f;
                    rawXBuff = 0.0f;
                    foreach (var item in MainWindow.ChartWin.PointMarkersSelectionModifier.SelectedPointMarkers)
                    {
                        if (rawXBuff != Convert.ToDouble(item.XValue))
                        {
                            rawXBuff = Convert.ToDouble(item.XValue);
                            x[i] = xv;                                  /*在遍历过程中填充x[]数组*/
                            y[i] = Convert.ToDouble(item.YValue);

                            i++;
                            xv += 0.2f;     /*0.2s一个数据*/
                        }
                        else
                        {
                            continue;
                        }
                    }
                    ///*添加寻找最大最小值*/
                    //ratio = FittingFunct.IndexEST(y, x);
                    Tuple<double, double> p;
                    p = Tout_TsetAndTimeCalculate(x, y);        /*返回一个result，理想曲线*/
                    ratio[0] = p.Item1;
                    ratio[1] = p.Item2;
                    /*以下为每个分段的指数系数和指数幂*/
                    if (index == 01)
                    {
                        record.a_c0 = Convert.ToSingle(ratio[0]);
                        record.k_c0 = Convert.ToSingle(ratio[1]);
                    }
                    else if (index == 10)
                    {
                        record.a_c1 = Convert.ToSingle(ratio[0]);
                        record.k_c1 = Convert.ToSingle(ratio[1]);
                    }
                    //System.Diagnostics.Debug.Print(ratio[0].ToString() + "--" + ratio[1].ToString());
                }
            }
        }
        #endregion

        /*function数据计算*/
        Tuple<double, double> Tout_TsetAndTimeCalculate(double[] x, double[] y)/*Tuple<double, double>*定义两个返回值find Ymax And Ymi。需要处理：此处需要添加Tset传入*/
        {
            Tuple<double, double> result;
            double dTimemax = 0.0f;
            double dTimemin = 0.0f;
            double Ymax = y[0];
            double Ymin = y[0];
            double dTempPositiveError;
            double dTimePositiveError;
            for (int iCount = 0; iCount < (x.Length - 1); iCount++)
            {

                if (y[iCount] > y[0])
                {
                    Ymax = y[iCount];
                    dTimemax = x[iCount];
                }
                if (y[iCount] < y[0])
                {
                    Ymin = y[iCount];
                    dTimemin = x[iCount];
                }
            }
            dTempPositiveError = Ymax - Ymin;
            dTimePositiveError = dTimemax - dTimemin;
            result = Tuple.Create(dTimePositiveError, dTempPositiveError);
            return result;
        }

        /*button1_Click事件：按键提示*/
        private void button1_Click(object sender, RoutedEventArgs e)
        {

            //SqlManager.UpdateProjectRecord(SqlManager.CUR_PROJECT_ID, record);
            System.Windows.MessageBox.Show("这已经是最后一步了，请继续完成分段调试！", "提示！");
            //this.NavigationService.Navigate(new Uri("pages/fuzzy.xaml", UriKind.Relative));/*JT20200701*/
            //this.NavigationService.Navigate(new Uri("pages/TriplePointSettingPage.xaml", UriKind.Relative));
        }
        private void buttonSave(object sender, RoutedEventArgs e)
        {

            SqlManager.UpdateProjectRecord(SqlManager.CUR_PROJECT_ID, record);
            System.Windows.MessageBox.Show("恭喜，调试结束！", "提示！");
            //this.NavigationService.Navigate(new Uri("pages/fuzzy.xaml", UriKind.Relative));/*JT20200701*/
            //this.NavigationService.Navigate(new Uri("pages/TriplePointSettingPage.xaml", UriKind.Relative));
        }


        /*private int subindexall;未使用*/
        private void fuzzyCheckBox_check(object sender, RoutedEventArgs e)
        {
            int TagFlag = 0;
            CheckBox rb = sender as CheckBox;
            TagFlag = (int)(rb.Tag);
            UpdateRecord(TagFlag);      /*每获得一次Tag，扫描所有*/
        }












        /*以下为整理之后的功能块*/


        /*JT加载数据*/
        private void LoadRecord()
        {
            List<ProjectRecord> records;

            records = SqlManager.SelectRecordFromeProjectTable(SqlManager.CUR_PROJECT_ID);
            record = records[0];

            this.DataContext = record;
        }

        /*UpdateRecord:记录checkBox 的当前选中状态*/
        private void UpdateRecord(int Tag)
        {
            int index;
            index = record.combination_num - 1;
            if (Tag < index)
            {
                for (int i = 0; i < index; i++)
                {
                    int v;
                    v = 0;

                    string str = "fd_" + i.ToString() + "_" + (i + 1).ToString();

                    if (checkBoxs[str].IsChecked == true)
                    {
                        v = 1;
                    }

                    if (i == 0)
                    {
                        record.state0_1 = v;
                    }
                    else if (i == 1)
                    {
                        record.state1_2 = v;
                    }
                    else if (i == 2)
                    {
                        record.state2_3 = v;
                    }
                    else
                    {
                        record.state3_4 = v;
                    }
                }
            }

            /*下切*/
            else
            {
                for (int i = 0; i < index; i++)/*每次进来都从0开始扫描一遍*/
                {
                    int v;
                    v = 0;

                    string str = "fd_" + (i + 1).ToString() + "_" + i.ToString();

                    if (checkBoxs[str].IsChecked == true)
                    {
                        v = 1;
                    }

                    if (i == 0)
                    {
                        record.state1_0 = v;
                    }
                    else if (i == 1)
                    {
                        record.state2_1 = v;
                    }
                    else if (i == 2)
                    {
                        record.state3_2 = v;
                    }
                    else
                    {
                        record.state4_3 = v;
                    }
                }
            }
        }

        /*UpdateCheckBox：根据record更新UI*/
        private void UpdateCheckBox(int Tag)
        {
            int subindex;
            subindex = record.combination_num - 1;
            if (Tag < subindex)
            {
                for (int i = 0; i < subindex; i++)
                {

                    int v;
                    if (i == 0)
                    {
                        v = record.state0_1;
                    }
                    else if (i == 1)
                    {
                        v = record.state1_2;
                    }
                    else if (i == 2)
                    {
                        v = record.state2_3;
                    }
                    else
                    {
                        v = record.state3_4;
                    }

                    string str = "fd_" + i.ToString() + "_" + (i + 1).ToString();
                    if ((v & 0x01) != 0)
                    {
                        checkBoxs[str].IsChecked = true;
                    }
                    else
                    {
                        checkBoxs[str].IsChecked = false;
                    }
                }
            }
            else
            {
                for (int i = 0; i < subindex; i++)
                {

                    int v;
                    if (i == 0)
                    {
                        v = record.state1_0;
                    }
                    else if (i == 1)
                    {
                        v = record.state2_1;
                    }
                    else if (i == 2)
                    {
                        v = record.state3_2;
                    }
                    else
                    {
                        v = record.state4_3;
                    }

                    string str = "fd_" + (i + 1).ToString() + "_" + i.ToString();
                    if ((v & 0x01) != 0)
                    {
                        checkBoxs[str].IsChecked = true;
                    }
                    else
                    {
                        checkBoxs[str].IsChecked = false;
                    }
                }
            }
        }

        /*创建UI*/
        Dictionary<string, CheckBox> checkBoxs;/*UI的checkBox盒子，通过string str区分每个checkBox*/
        private void CreateUI()
        {

            int indexfd;
            int updown;
            indexfd = record.combination_num - 1;
            if (stackPanelfuzzy == null)        /*整个页面的子容器*/
            {
                return;
            }

            stackPanelfuzzy.Children.Clear();
            checkBoxs.Clear();
            /****************************************************************************************************/
            /* 表头 */
            /*StackPanel stackPanelHeader = new StackPanel();
            stackPanelHeader.Orientation = Orientation.Horizontal;
            stackPanelHeader.HorizontalAlignment = HorizontalAlignment.Center;
            stackPanelHeader.VerticalAlignment = VerticalAlignment.Center;

            Label labelmotion = new Label();
            
            labelmotion.Content = "分段动作";
            labelmotion.Width = 80;
            labelmotion.Margin = new Thickness(50, 4, -15, 4);
            stackPanelHeader.Children.Add(labelmotion);

            Label labelPwmTime = new Label();
            labelPwmTime.Content = "PWM调整";
            labelPwmTime.Width = 80;
            labelPwmTime.Margin = new Thickness(5, 4, 14, 4);
            stackPanelHeader.Children.Add(labelPwmTime);

            Label labelAddab = new Label();
            labelAddab.Content = "是否重叠燃烧";
            labelAddab.Width = 80;
            labelAddab.Margin = new Thickness(40, 4, 4, 4);
            stackPanelHeader.Children.Add(labelAddab);

            Label labelabTime = new Label();
            labelabTime.Content = "重叠时间";
            labelabTime.Width = 80;
            labelabTime.Margin = new Thickness(0, 4, 14, 4);
            stackPanelHeader.Children.Add(labelabTime);

            stackPanelfuzzy.Children.Add(stackPanelHeader);*/
            /****************************************************************************************************/
            StackPanel stackPanelHeader = new StackPanel();
            stackPanelHeader.Orientation = Orientation.Horizontal;
            stackPanelHeader.HorizontalAlignment = HorizontalAlignment.Center;
            stackPanelHeader.VerticalAlignment = VerticalAlignment.Center;

            Label labelRunPoint = new Label();
            labelRunPoint.Content = "Run Point";
            labelRunPoint.Width = 80;
            labelRunPoint.Margin = new Thickness(14, 4, 14, 4);
            stackPanelHeader.Children.Add(labelRunPoint);

            Label labelPwm = new Label();
            labelPwm.Content = "PWM";
            labelPwm.Width = 80;
            labelPwm.Margin = new Thickness(14, 4, 14, 4);
            stackPanelHeader.Children.Add(labelPwm);

            Label labelDuty = new Label();
            labelDuty.Content = "Duty";
            labelDuty.Width = 80;
            labelDuty.Margin = new Thickness(14, 4, 14, 4);
            stackPanelHeader.Children.Add(labelDuty);

            Label labelPa = new Label();
            labelPa.Content = "Pa";
            labelPa.Width = 80;
            labelPa.Margin = new Thickness(14, 4, 14, 4);
            stackPanelHeader.Children.Add(labelPa);
            stackPanelfuzzy.Children.Add(stackPanelHeader);
            /*上切分段*/
            //stackPanelPoints.Children.RemoveRange(0, stackPanelPoints.Children.Count);
            for (int i = 0; i < indexfd; i++)
            {
                updown = 1;
                StackPanel sp = new StackPanel();
                sp.Orientation = Orientation.Horizontal;
                sp.VerticalAlignment = VerticalAlignment.Center;
                sp.HorizontalAlignment = HorizontalAlignment.Center;

                /*重叠选项是否开启,checkBox*/
                CheckBox checkBox = new CheckBox();
                checkBox.Tag = i;
                checkBox.Content = "重叠燃烧";
                checkBox.Margin = new Thickness(75, 8, 14, 4);
                string str = "fd_" + i.ToString() + "_" + (i + 1).ToString();
                checkBox.Click += fuzzyCheckBox_check;/*事件*/
                checkBoxs.Add(str, checkBox);

                /*重叠时间，int型SingleUpDown t = time*10 */
                //Label labelfdup = new Label();/*名称标签*/
                //labelfdup.Content = "重叠时长0.1s";/*标签显示内容*/
                //labelfdup.Margin = new Thickness(14, 4, 14, 4);
                SingleUpDown singleUpDownfdup = new SingleUpDown();
                singleUpDownfdup.Width = 60;
                singleUpDownfdup.Increment = 1f;
                singleUpDownfdup.Maximum = 50;
                singleUpDownfdup.Minimum = 0;
                Binding bindingfd01 = new Binding("sub_over_time_c" + i.ToString() + "_" + (i + 1).ToString());/**/
                bindingfd01.Mode = BindingMode.TwoWay;
                singleUpDownfdup.SetBinding(SingleUpDown.ValueProperty, bindingfd01);

                /*缓动时间，int型SingleUpDown t = time*10 */
                Label labelfduppwm = new Label();/*名称标签*/
                labelfduppwm.Margin = new Thickness(16, 4, 14, 4);
                labelfduppwm.Content = "分段" + i.ToString() + "-" + (i + 1).ToString();/*标签显示内容*/
                SingleUpDown singleUpDownfdpwm = new SingleUpDown();
                singleUpDownfdpwm.Width = 60;
                singleUpDownfdpwm.Increment = 1f;
                singleUpDownfdpwm.Maximum = 50;
                singleUpDownfdpwm.Minimum = -30;
                Binding bindingfdupPwm = new Binding("pwm_tween_c" + i.ToString() + "_" + (i + 1).ToString());
                bindingfdupPwm.Mode = BindingMode.TwoWay;
                singleUpDownfdpwm.SetBinding(SingleUpDown.ValueProperty, bindingfdupPwm);

                sp.Children.Add(labelfduppwm);
                sp.Children.Add(singleUpDownfdpwm);

                sp.Children.Add(checkBox);

                sp.Children.Add(singleUpDownfdup);
                //sp.Children.Add(labelfdup);



                stackPanelfuzzy.Children.Add(sp);
            }
            /*下切分段*/
            for (int i = 0; i < indexfd; i++)
            {
                updown = 10;
                StackPanel sp = new StackPanel();
                sp.Orientation = Orientation.Horizontal;
                sp.VerticalAlignment = VerticalAlignment.Center;
                sp.HorizontalAlignment = HorizontalAlignment.Center;

                CheckBox checkBox = new CheckBox();
                checkBox.Tag = i + indexfd;
                checkBox.Content = "重叠燃烧";
                checkBox.Margin = new Thickness(75, 8, 14, 4);
                string str = "fd_" + (i + 1).ToString() + "_" + i.ToString();
                checkBox.Click += fuzzyCheckBox_check;/*事件*/
                checkBoxs.Add(str, checkBox);

                //Label labelfdDown = new Label();/*名称标签*/
                //labelfdDown.Margin = new Thickness(0, 4, 14, 4);
                //labelfdDown.Content = "重叠时长" + (i + 1).ToString() + "-" + i.ToString();/*标签显示内容*/

                SingleUpDown singleUpDownfd011 = new SingleUpDown();
                singleUpDownfd011.Width = 60;
                singleUpDownfd011.Increment = 1f;
                singleUpDownfd011.Maximum = 50;
                singleUpDownfd011.Minimum = 0;
                Binding bindingfd01 = new Binding("sub_over_time_c" + (i + 1).ToString() + "_" + i.ToString());
                bindingfd01.Mode = BindingMode.TwoWay;
                singleUpDownfd011.SetBinding(SingleUpDown.ValueProperty, bindingfd01);

                /*缓动时间，int型SingleUpDown t = time*10 */
                Label labelfddownpwm = new Label();/*名称标签*/
                labelfddownpwm.Margin = new Thickness(16, 4, 14, 4);
                labelfddownpwm.Content = "分段" + (i + 1).ToString() + "-" + i.ToString();/*标签显示内容*/
                SingleUpDown singleUpDownfdpwm = new SingleUpDown();
                singleUpDownfdpwm.Width = 60;
                singleUpDownfdpwm.Increment = 1f;
                singleUpDownfdpwm.Maximum = 50;
                singleUpDownfdpwm.Minimum = -50;
                //singleUpDownfdpwm.Margin = new Thickness(16, 4, 0, 4);
                Binding bindingfdupPwm = new Binding("pwm_tween_c" + (i + 1).ToString() + "_" + i.ToString());
                bindingfdupPwm.Mode = BindingMode.TwoWay;
                singleUpDownfdpwm.SetBinding(SingleUpDown.ValueProperty, bindingfdupPwm);


                sp.Children.Add(labelfddownpwm);
                sp.Children.Add(singleUpDownfdpwm);

                sp.Children.Add(checkBox);

                sp.Children.Add(singleUpDownfd011);
                //sp.Children.Add(labelfdDown);




                stackPanelfuzzy.Children.Add(sp);
            }
            Button Button1Save = new Button();
            Button1Save.Content = "Save";

            Button1Save.Click += buttonSave;

            stackPanelfuzzy.Children.Add(Button1Save);
            UpdateCheckBox(indexfd);
        }
    }
}
