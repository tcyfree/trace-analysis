/*****************************************************************************************************************************
This project was supported by the National Basic Research 973 Program of China under Grant No.2011CB302301
Huazhong University of Science and Technology (HUST)   Wuhan National Laboratory for Optoelectronics

FileName： ssd.c
Author: Hu Yang        Version: 2.1    Date:2011/12/02
Description:

History:
<contributor>     <time>        <version>       <desc>                   <e-mail>
Yang Hu            2009/09/25          1.0            Creat SSDsim       yanghu@foxmail.com
                2010/05/01        2.x           Change
Zhiming Zhu     2011/07/01        2.0           Change               812839842@qq.com
Shuangwu Zhang  2011/11/01        2.1           Change               820876427@qq.com
Chao Ren        2011/07/01        2.0           Change               529517386@qq.com
Hao Luo         2011/01/01        2.0           Change               luohao135680@gmail.com
*****************************************************************************************************************************/

#include "ssd.h"

/********************************************************************************************************************************
1，main函数中initiatio()函数用来初始化ssd,；2，make_aged()函数使SSD成为aged，aged的ssd相当于使用过一段时间的ssd，里面有失效页，
non_aged的ssd是新的ssd，无失效页，失效页的比例可以在初始化参数中设置；3，pre_process_page()函数提前扫一遍读请求，把读请求
的lpn<--->ppn映射关系事先建立好，写请求的lpn<--->ppn映射关系在写的时候再建立，预处理trace防止读请求是读不到数据；4，simulate()是
核心处理函数，trace文件从读进来到处理完成都由这个函数来完成；5，statistic_output()函数将ssd结构中的信息输出到输出文件，输出的是
统计数据和平均数据，输出文件较小，trace_output文件则很大很详细；6，free_all_node()函数释放整个main函数中申请的节点
*********************************************************************************************************************************/
int main(int argc, char *argv[])
{
    unsigned int i, j, k;
    struct ssd_info *ssd;

#ifdef DEBUG
    printf("enter main\n");
#endif

    ssd = (struct ssd_info *)malloc(sizeof(struct ssd_info));
    alloc_assert(ssd, "ssd");
    memset(ssd, 0, sizeof(struct ssd_info));

    //*****************************************************
    int sTIMES, speed_up;
    speed_up = 1;
    printf("Read parameters to the main function.\n");
    sscanf(argv[1], "%d", &sTIMES);
    //sscanf(argv[2], "%d", &speed_up);
    sscanf(argv[2], "%s", &(ssd->tracefilename));
    printf("Running trace file: %s.\n", ssd->tracefilename);
    //*****************************************************

    ssd = initiation(ssd);
    printf("Chip_channel: %d, %d\n", ssd->parameter->chip_channel[0], ssd->parameter->chip_num); //（各channel上chip数量，整个SSD上chip数量）
    make_aged(ssd);
    pre_process_page(ssd);
    //get_old_zwh(ssd);
    printf("free_lsb: %d, free_csb: %d, free_msb: %d\n", ssd->free_lsb_count, ssd->free_csb_count, ssd->free_msb_count);
    printf("Total request num: %lld.\n", ssd->total_request_num);
    for (i = 0; i < ssd->parameter->channel_number; i++)
    {
        for (j = 0; j < ssd->parameter->die_chip; j++)
        {
            for (k = 0; k < ssd->parameter->plane_die; k++)
            {
                printf("%d,0,%d,%d:  %5d\n", i, j, k, ssd->channel_head[i].chip_head[0].die_head[j].plane_head[k].free_page);
            }
        }
    }
    fprintf(ssd->outputfile, "\t\t\t\t\t\t\t\t\tOUTPUT\n");
    fprintf(ssd->outputfile, "****************** TRACE INFO ******************\n");
    //********************************************

    //sscanf(argv[3], "%s", &
    if (speed_up <= 0)
    {
        printf("Parameter ERROR.\n");
        return 0;
    }
    printf("RUN %d TIMES with %dx SPEED.\n", sTIMES, speed_up);
    if (ssd->parameter->fast_gc == 1)
    {
        printf("Fast GC is activated, %.2f:%.2f and %.2f:%.2f.\n", ssd->parameter->fast_gc_thr_1, ssd->parameter->fast_gc_filter_1, ssd->parameter->fast_gc_thr_2, ssd->parameter->fast_gc_filter_2);
    }
    printf("dr_switch: %d, dr_cycle: %d\n", ssd->parameter->dr_switch, ssd->parameter->dr_cycle);
    if (ssd->parameter->dr_switch == 1)
    {
        printf("Data Reorganization is activated, dr cycle is %d days.\n", ssd->parameter->dr_cycle);
    }
    printf("GC_hard threshold: %.2f.\n", ssd->parameter->gc_hard_threshold);
    ssd->speed_up = speed_up;
    //*********************************************
    ssd=simulate(ssd);
    srand((unsigned int)time(NULL));
    //ssd = simulate_multiple(ssd, sTIMES);
    statistic_output(ssd);

    printf("\n");
    printf("request count: %lld\n",ssd->read_req_trace+ssd->write_req_trace);
    printf("write ratio: %.4f\n", (double) ssd->write_req_trace / (ssd->read_req_trace + ssd->write_req_trace));
    printf("write size(KB): %.4f\n", (double)ssd->write_size_trace / ssd->write_req_trace / 2);
    printf("\n");

    printf("average response time: %lld\n", (ssd->write_avg + ssd->read_avg) / (ssd->write_request_count + ssd->read_request_count));
    printf("buffer hits: %lu\n", ssd->all_page_count - ssd->dram->buffer->read_miss_hit - ssd->dram->buffer->write_miss_hit);
    //printf("pop of every time: %lu\n", ssd->dram->buffer->write_miss_hit / ssd->pop_count);
    //printf("buffer_page_count: %lu\n", ssd->buffer_page[0] / ssd->buffer_page[1]);
    printf("write flash: %lu\n", ssd->dram->buffer->write_miss_hit);
    printf("all page: %lu\n", ssd->all_page_count);

    printf("\n");
    printf("the simulation is completed!\n");

    //写入文件记录
    FILE * fp = NULL;
    fp = fopen("./trace-analysis.txt","w+");
    // fprintf(ssd->outputfile,"erase: %13d\n",erase);
    // fprintf(fp, "request count: %lld\n", ssd->read_req_trace+ssd->write_req_trace);
    fprintf(fp, "write ratio: %.4f,", (double) ssd->write_req_trace / (ssd->read_req_trace + ssd->write_req_trace));
    fprintf(fp, "total request num: %d,", ssd->read_req_trace + ssd->write_req_trace);
    fprintf(fp, "average write size(KB): %.4f\n", (double)ssd->write_size_trace / ssd->write_req_trace / 2);
    fclose(fp);

    return 0;
    /*     _CrtDumpMemoryLeaks(); */
}

/******************simulate() *********************************************************************
*simulate()是核心处理函数，主要实现的功能包括
*1,从trace文件中获取一条请求，挂到ssd->request
*2，根据ssd是否有dram分别处理读出来的请求，把这些请求处理成为读写子请求，挂到ssd->channel或者ssd上
*3，按照事件的先后来处理这些读写子请求。
*4，输出每条请求的子请求都处理完后的相关信息到outputfile文件中
**************************************************************************************************/
struct ssd_info *simulate(struct ssd_info *ssd)
{
    int flag = 1, flag1 = 0;
    double output_step = 0;
    unsigned int a = 0, b = 0;
    //errno_t err;

    printf("\n");
    printf("begin simulating.......................\n");
    printf("\n");
    printf("\n");
    printf("   ^o^    OK, please wait a moment, and enjoy music and coffee   ^o^    \n");

    ssd->tracefile = fopen(ssd->tracefilename, "r");
    if (ssd->tracefile == NULL)
    {
        printf("the trace file can't open\n");
        return NULL;
    }

    fprintf(ssd->outputfile, "      arrive           lsn     size ope     begin time    response time    process time\n");
    fflush(ssd->outputfile);

    while (flag != 100)
    {

        flag = get_requests(ssd);

        if (flag == 1)
        {
            //printf("once\n");
            if (ssd->parameter->dram_capacity != 0)
            {
                buffer_management(ssd);

                /*读请求分配子请求函数，这里只处理读请求，
                写请求已经在buffer_management()函数中处理了根据请求队列和buffer命中的检查，
                将每个请求分解成子请求，将子请求队列挂在channel上，不同的channel有自己的子请求队列。*/
                //distribute(ssd);
            }
            else
            {
                no_buffer_distribute(ssd);
            }
        }

        process(ssd);
        trace_output(ssd);
        if (flag == 0)
            flag = 100;
    }

    fclose(ssd->tracefile);
    return ssd;
}



/********    get_request    ******************************************************
*    1.get requests that arrived already
*    2.add those request node to ssd->reuqest_queue
*    return    0: reach the end of the trace
*            -1: no request has been added
*            1: add one request to list
*SSD模拟器有三种驱动方式:时钟驱动(精确，太慢) 事件驱动(本程序采用) trace驱动()，
*两种方式推进事件：channel/chip状态改变、trace文件请求达到。
*channel/chip状态改变和trace文件请求到达是散布在时间轴上的点，每次从当前状态到达
*下一个状态都要到达最近的一个状态，每到达一个点执行一次process
********************************************************************************/
int get_requests(struct ssd_info *ssd)
{
    char buffer[200];
    unsigned int lsn = 0;
    int device, size, ope, large_lsn, i = 0, j = 0;
    int priority;
    struct request *request1; //I/O请求项
    int flag = 1;
    long filepoint;             //文件指针偏移量
    int64_t time_t = 0;         //请求到达时间（即时间戳）
    int64_t nearest_event_time; //寻找所有子请求的最早到达下个状态的时间

#ifdef DEBUG
    printf("enter get_requests,  current time:%lld\n", ssd->current_time);
#endif

    if (feof(ssd->tracefile))
        return 0;

    filepoint = ftell(ssd->tracefile);
    fgets(buffer, 200, ssd->tracefile);                                                    //从trace文件中读取一行内容至缓冲区
    sscanf(buffer, "%lld %d %d %d %d %d", &time_t, &device, &lsn, &size, &ope, &priority); //按照I/O格式将每行读取的内容中的参数读到time_t，device等中
    //printf("%lld\n", time_t);
    //=======================================
    priority = 1; //强制设定优先级
    time_t = time_t / (ssd->speed_up);
    time_t = ssd->basic_time + time_t;
    //=======================================
    if ((device < 0) && (lsn < 0) && (size < 0) && (ope < 0)) //检查这个请求的I/O参数的合理性
    {
        return 100; //不合理返回100，然后赋值给flag退出循环
    }
    //若参数合法，则调整对应的参数（防止不合理）
    if (size == 0)
    {
        size = 1;
    }
    if (lsn < ssd->min_lsn)
        ssd->min_lsn = lsn;
    if (lsn > ssd->max_lsn)
        ssd->max_lsn = lsn;
    /******************************************************************************************************
    *上层文件系统发送给SSD的任何读写命令包括两个部分（LSN，size） LSN是逻辑扇区号，对于文件系统而言，它所看到的存
    *储空间是一个线性的连续空间。例如，读请求（260，6）表示的是需要读取从扇区号为260的逻辑扇区开始，总共6个扇区。
    *large_lsn: channel下面有多少个subpage，即多少个sector。overprovide系数：SSD中并不是所有的空间都可以给用户使用，
    *比如32G的SSD可能有10%的空间保留下来留作他用，所以乘以1-provide
    ***********************************************************************************************************/
    //（求一个ssd最大的逻辑扇区号，因为chip_num是ssd的prarmeter_value里面的成员（文件initialize.h里面））
    large_lsn = (int)((ssd->parameter->subpage_page * ssd->parameter->page_block * ssd->parameter->block_plane * ssd->parameter->plane_die * ssd->parameter->die_chip * ssd->parameter->chip_num) * (1 - ssd->parameter->overprovide));
    //根据当前lsn参数来确定是第几个lsn，也就是具体的逻辑扇区号
    lsn = lsn % large_lsn;

    nearest_event_time = find_nearest_event(ssd); //寻找所有子请求中最早到达下一状态的时间
    if (nearest_event_time == MAX_INT64)          //判断这个值是否等于转换到下一个状态的时间极限值
    {
        ssd->current_time = time_t; //如果是，（说明此时获取不到当前IOtrace合理的下一状态预计时间，但是该IO请求是可以进行处理的，只是下一状态预计时间为最大值，I/O会一直被阻塞）则把系统当前时间更新为I/O到达时间（表示I/O已经处理）

        //if (ssd->request_queue_length>ssd->parameter->queue_length)    //如果请求队列的长度超过了配置文件中所设置的长度
        //{
        //printf("error in get request , the queue length is too long\n");
        //}
    }
    else //则nearest_event_time是合理且可用的，表示下一状态是可以到达的
    {
        if (nearest_event_time < time_t) //判断I/O实际到达时间是否大于预计到达时间（若是则表明I/O处于阻塞状态）
        {
            /*******************************************************************************
            *回滚，即如果没有把time_t赋给ssd->current_time，则trace文件已读的一条记录回滚
            *filepoint记录了执行fgets之前的文件指针位置，回滚到文件头+filepoint处
            *int fseek(FILE *stream, long offset, int fromwhere);函数设置文件指针stream的位置。
            *如果执行成功，stream将指向以fromwhere（偏移起始位置：文件头0，当前位置1，文件尾2）为基准，
            *偏移offset（指针偏移量）个字节的位置。如果执行失败(比如offset超过文件自身大小)，则不改变stream指向的位置。
            *文本文件只能采用文件头0的定位方式，本程序中打开文件方式是"r":以只读方式打开文本文件
            **********************************************************************************/
            fseek(ssd->tracefile, filepoint, 0);
            if (ssd->current_time <= nearest_event_time)
                ssd->current_time = nearest_event_time;
            return -1;
        }
        else
        {
            if (ssd->request_queue_length >= ssd->parameter->queue_length) //判断ssd中I/O请求对流是否已满
            {
                fseek(ssd->tracefile, filepoint, 0);    //满了，就回滚重置文件指针
                ssd->current_time = nearest_event_time; //且设置系统时间为下一状态预计到达时间
                return -1;
            }
            else //未满，证明此时当前的IO请求是可以正常插入请求队列
            {
                ssd->current_time = time_t;
            }
        }
    }

    if (time_t < 0) //若IO请求的处理时间非法
    {
        printf("error!\n");
        while (1)
        {
        }
    }

    if (feof(ssd->tracefile)) //判断是否已经读取完了trace文件中所有的I/Otrace
    {
        //判断结果是已经读取完了，则置空request1，并返回函数
        request1 = NULL;
        return 0;
    }
    //若没有读取完，执行到此说明该IO是合理并且可以插入队列
    request1 = (struct request *)malloc(sizeof(struct request));
    alloc_assert(request1, "request");
    memset(request1, 0, sizeof(struct request));

    request1->time = time_t;
    request1->lsn = lsn;
    request1->size = size;
    request1->operation = ope;
    request1->priority = priority; //（）设置为不空闲
    request1->begin_time = time_t;
    request1->response_time = 0;
    request1->energy_consumption = 0;
    request1->next_node = NULL;
    request1->distri_flag = 0; // indicate whether this request has been distributed already
    request1->subs = NULL;
    request1->need_distr_flag = NULL;
    request1->complete_lsn_count = 0;  //record the count of lsn served by buffer
    filepoint = ftell(ssd->tracefile); // set the file point

    if (ssd->request_queue == NULL) //The queue is empty
    {
        ssd->request_queue = request1;
        ssd->request_tail = request1;
        ssd->request_queue_length++;
    }
    else
    {
        (ssd->request_tail)->next_node = request1;
        ssd->request_tail = request1;
        ssd->request_queue_length++;
    }
    if (ssd->request_queue_length > ssd->max_queue_depth)
    {
        ssd->max_queue_depth = ssd->request_queue_length;
    }
    
    if (request1->operation == 1) //计算平均请求大小 1为读 0为写
    {
        ssd->read_req_trace++;
        ssd->read_size_trace+=size;
    }
    else
    {
        ssd->write_req_trace++;
        ssd->write_size_trace += size;
    }

    filepoint = ftell(ssd->tracefile);  //更新文件偏移量
    fgets(buffer, 200, ssd->tracefile); //寻找下一条请求的到达时间（继续读取下一条I/Otrace）
    sscanf(buffer, "%lld %d %d %d %d", &time_t, &device, &lsn, &size, &ope);
    ssd->next_request_time = time_t;
    fseek(ssd->tracefile, filepoint, 0);

    return 1;
}

/*****************************
*lpn向ppn的转换，
******************************/
unsigned int lpn2ppn(struct ssd_info *ssd, unsigned int lsn)
{
    int lpn, ppn;
    struct entry *p_map = ssd->dram->map->map_entry; //获取映射表
#ifdef DEBUG
    printf("enter lpn2ppn,  current time:%lld\n", ssd->current_time);
#endif
    lpn = lsn / ssd->parameter->subpage_page; //subpage_page指一个page中包含的子页数量，在参数文件中可以设定
    ppn = (p_map[lpn]).pn;                    //逻辑页lpn和物理页ppn的映射记录在映射表中
    return ppn;
}


/**********************************************************************
*trace_output()函数是在每一条请求的所有子请求经过process()函数处理完后，
*打印输出相关的运行结果到outputfile文件中，这里的结果主要是运行的时间
**********************************************************************/
void trace_output(struct ssd_info *ssd)
{
    int flag = 1;
    int64_t start_time, end_time;
    struct request *req, *pre_node;
    struct sub_request *sub, *tmp;

#ifdef DEBUG
    printf("enter trace_output,  current time:%lld\n", ssd->current_time);
#endif
    int debug_0918 = 0;
    pre_node = NULL;
    req = ssd->request_queue;
    start_time = 0;
    end_time = 0;

    if (req == NULL)
        return;

    while (req != NULL)
    {
        sub = req->subs;
        flag = 1;
        start_time = 0;
        end_time = 0;
        if (req->response_time != 0)
        {
            //printf("Rsponse time != 0?\n");
            fprintf(ssd->outputfile, "%16lld %10d %6d %2d %16lld %16lld %10lld\n", req->time, req->lsn, req->size, req->operation, req->begin_time, req->response_time, req->response_time - req->time);
            fflush(ssd->outputfile);
            ssd->completed_request_count++;

            if (ssd->completed_request_count % 100000 == 0)
            {
                ssd->buffer_page[0] += ssd->dram->buffer->current_page_count;
                ssd->buffer_page[1]++;
            }

            if (ssd->completed_request_count % 10000 == 0)
            {
                // printf("completed requests: %d\n", ssd->completed_request_count);
                statistic_output_easy(ssd, ssd->completed_request_count);
                ssd->newest_read_avg = 0;
                ssd->newest_write_avg = 0;
                ssd->newest_read_request_count = 0;
                ssd->newest_write_request_count = 0;
                ssd->newest_write_lsb_count = 0;
                ssd->newest_write_csb_count = 0;
                ssd->newest_write_msb_count = 0;
                ssd->newest_write_request_completed_with_same_type_pages = 0;
                //***************************************************************************
                int channel;
                int this_day;
                this_day = (int)(ssd->current_time / 1000000000 / 3600 / 24);
                /*
                if(this_day>ssd->time_day){
                    printf("Another Day begin, %d.\n", this_day);
                    }
                    */
                if (this_day > ssd->time_day)
                {
                    printf("Day %d begin......\n", this_day);
                    ssd->time_day = this_day;
                    if ((ssd->parameter->dr_switch == 1) && (this_day % ssd->parameter->dr_cycle == 0))
                    {
                        for (channel = 0; channel < ssd->parameter->channel_number; channel++)
                        {
                            dr_for_channel(ssd, channel);
                        }
                    }
                    /*
                    if((ssd->parameter->turbo_mode==2)&&(this_day%7==3)){
                        printf("Enter turbo-mode.....\n");
                        ssd->parameter->lsb_first_allocation = 1;
                        ssd->parameter->fast_gc = 1;
                        }
                    else if(ssd->parameter->turbo_mode==2){
                        //printf("Exist turbo-mode....\n");
                        ssd->parameter->lsb_first_allocation = 0;
                        ssd->parameter->fast_gc = 0;
                        }
                        */
                }
                //***************************************************************************
            }

            if (req->response_time - req->begin_time == 0)
            {
                printf("the response time is 0?? \n");
                getchar();
            }

            if (req->operation == READ)
            {
                ssd->read_request_count++;
                ssd->read_avg += (req->response_time - req->time);
                //===========================================
                ssd->newest_read_request_count++;
                ssd->newest_read_avg = ssd->newest_read_avg + (end_time - req->time);
                //===========================================
            }
            else
            {
                ssd->write_request_count++;
                ssd->write_avg += (req->response_time - req->time);
                //===========================================
                ssd->newest_write_request_count++;
                ssd->newest_write_avg = ssd->newest_write_avg + (end_time - req->time);
                ssd->last_write_lat = end_time - req->time;
                //===========================================
            }

            if (pre_node == NULL)
            {
                if (req->next_node == NULL)
                {
                    free(req->need_distr_flag);
                    req->need_distr_flag = NULL;
                    free(req);
                    req = NULL;
                    ssd->request_queue = NULL;
                    ssd->request_tail = NULL;
                    ssd->request_queue_length--;
                }
                else
                {
                    ssd->request_queue = req->next_node;
                    pre_node = req;
                    req = req->next_node;
                    free(pre_node->need_distr_flag);
                    pre_node->need_distr_flag = NULL;
                    free((void *)pre_node);
                    pre_node = NULL;
                    ssd->request_queue_length--;
                }
            }
            else
            {
                if (req->next_node == NULL)
                {
                    pre_node->next_node = NULL;
                    free(req->need_distr_flag);
                    req->need_distr_flag = NULL;
                    free(req);
                    req = NULL;
                    ssd->request_tail = pre_node;
                    ssd->request_queue_length--;
                }
                else
                {
                    pre_node->next_node = req->next_node;
                    free(req->need_distr_flag);
                    req->need_distr_flag = NULL;
                    free((void *)req);
                    req = pre_node->next_node;
                    ssd->request_queue_length--;
                }
            }
        }
        else
        {
            //printf("Rsponse time = 0!\n");
            flag = 1;
            while (sub != NULL)
            {
                if (start_time == 0)
                    start_time = sub->begin_time;
                if (start_time > sub->begin_time)
                    start_time = sub->begin_time;
                if (end_time < sub->complete_time)
                    end_time = sub->complete_time;
                if ((sub->current_state == SR_COMPLETE) || ((sub->next_state == SR_COMPLETE) && (sub->next_state_predict_time <= ssd->current_time))) // if any sub-request is not completed, the request is not completed
                {
                    sub = sub->next_subs;
                }
                else
                {
                    flag = 0;
                    break;
                }
            }

            if (flag == 1)
            {
                //fprintf(ssd->outputfile,"%10I64u %10u %6u %2u %16I64u %16I64u %10I64u\n",req->time,req->lsn, req->size, req->operation, start_time, end_time, end_time-req->time);
                fprintf(ssd->outputfile, "%16lld %10d %6d %2d %16lld %16lld %10lld\n", req->time, req->lsn, req->size, req->operation, start_time, end_time, end_time - req->time);
                fflush(ssd->outputfile);
                ssd->completed_request_count++;

                if (ssd->completed_request_count % 100000 == 0)
                {
                    ssd->buffer_page[0] += ssd->dram->buffer->current_page_count;
                    ssd->buffer_page[1]++;
                }

                if (ssd->completed_request_count % 10000 == 0)
                {
                    printf("completed requests: %d, max_queue_depth: %d, ", ssd->completed_request_count, ssd->max_queue_depth);
                    printf("free_lsb: %d, free_csb: %d, free_msb: %d\n", ssd->free_lsb_count, ssd->free_csb_count, ssd->free_msb_count);
                    ssd->max_queue_depth = 0;
                    statistic_output_easy(ssd, ssd->completed_request_count);
                    ssd->newest_read_avg = 0;
                    ssd->newest_write_avg = 0;
                    ssd->newest_write_avg_l = 0;
                    ssd->newest_read_request_count = 0;
                    ssd->newest_write_request_count = 0;
                    ssd->newest_write_lsb_count = 0;
                    ssd->newest_write_csb_count = 0;
                    ssd->newest_write_msb_count = 0;
                    ssd->newest_write_request_completed_with_same_type_pages_l = 0;
                    ssd->newest_write_request_num_l = 0;
                    ssd->newest_req_with_lsb_l = 0;
                    ssd->newest_req_with_csb_l = 0;
                    ssd->newest_req_with_msb_l = 0;
                    ssd->newest_write_request_completed_with_same_type_pages = 0;
                    ssd->newest_req_with_lsb = 0;
                    ssd->newest_req_with_csb = 0;
                    ssd->newest_req_with_msb = 0;
                    //***************************************************************************
                    int channel;
                    int this_day;
                    this_day = (int)(ssd->current_time / 1000000000 / 3600 / 24);
                    if (this_day > ssd->time_day)
                    {
                        printf("Day %d begin......\n", this_day);
                        ssd->time_day = this_day;
                        if ((ssd->parameter->dr_switch == 1) && (this_day % ssd->parameter->dr_cycle == 0))
                        {
                            for (channel = 0; channel < ssd->parameter->channel_number; channel++)
                            {
                                dr_for_channel(ssd, channel);
                            }
                        }
                        /*
                        if((ssd->parameter->turbo_mode==2)&&(this_day%2==1)){
                            printf("Enter turbo-mode.....\n");
                            ssd->parameter->lsb_first_allocation = 1;
                            ssd->parameter->fast_gc = 1;
                            }
                        else{
                            //printf("Exist turbo-mode....\n");
                            ssd->parameter->lsb_first_allocation = 0;
                            ssd->parameter->fast_gc = 0;
                            }
                            */
                    }
                    //***************************************************************************
                }

                if (debug_0918)
                {
                    // printf("completed requests: %d\n", ssd->completed_request_count);
                }
                if (end_time - start_time == 0)
                {
                    printf("the response time is 0?? position 2\n");
                    //getchar();
                }
                if (req->operation == READ)
                {
                    ssd->read_request_count++;
                    ssd->read_avg += (end_time - req->time);
                    //=============================================
                    ssd->newest_read_request_count++;
                    ssd->newest_read_avg = ssd->newest_read_avg + (end_time - req->time);
                    //==============================================
                }
                else
                {
                    ssd->write_request_count++;
                    ssd->write_avg += (end_time - req->time);
                    //=============================================
                    ssd->newest_write_request_count++;
                    ssd->newest_write_avg = ssd->newest_write_avg + (end_time - req->time);
                    ssd->last_write_lat = end_time - req->time;
                    ssd->last_ten_write_lat[ssd->write_lat_anchor] = end_time - req->time;
                    ssd->write_lat_anchor = (ssd->write_lat_anchor + 1) % 10;

                    //--------------------------------------------
                    int new_flag = 1;
                    int origin, actual_type;
                    int num_of_sub = 1;
                    struct sub_request *next_sub_a;
                    next_sub_a = req->subs;
                    origin = next_sub_a->allocated_page_type;
                    actual_type = next_sub_a->allocated_page_type;
                    next_sub_a = next_sub_a->next_subs;
                    while (next_sub_a != NULL)
                    {
                        num_of_sub++;
                        if (next_sub_a->allocated_page_type > actual_type)
                        {
                            actual_type = next_sub_a->allocated_page_type;
                        }
                        if (next_sub_a->allocated_page_type != origin)
                        {
                            new_flag = 0;
                        }
                        next_sub_a = next_sub_a->next_subs;
                    }
                    if (num_of_sub > 1)
                    {
                        ssd->write_request_count_l++;
                        ssd->newest_write_request_num_l++;
                        ssd->newest_write_avg_l = ssd->newest_write_avg_l + (end_time - req->time);
                        ssd->write_avg_l = ssd->write_avg_l + (end_time - req->time);
                    }
                    if (new_flag == 1)
                    {
                        ssd->newest_write_request_completed_with_same_type_pages++;
                        if (num_of_sub > 1)
                        {
                            ssd->newest_write_request_completed_with_same_type_pages_l++;
                        }
                        if (origin == 1)
                        {
                            ssd->newest_msb_request_a++;
                        }
                        else if (origin == 0)
                        {
                            ssd->newest_lsb_request_a++;
                        }
                        else
                        {
                            ssd->newest_csb_request_a++;
                        }
                    }
                    if (actual_type == TARGET_LSB)
                    {
                        ssd->newest_req_with_lsb++;
                        if (num_of_sub > 1)
                        {
                            ssd->newest_req_with_lsb_l++;
                        }
                    }
                    else if (actual_type == TARGET_CSB)
                    {
                        ssd->newest_req_with_csb++;
                        if (num_of_sub > 1)
                        {
                            ssd->newest_req_with_csb_l++;
                        }
                    }
                    else
                    {
                        ssd->newest_req_with_msb++;
                        if (num_of_sub > 1)
                        {
                            ssd->newest_req_with_msb_l++;
                        }
                    }
                    //-------------------------------------------

                    //==============================================
                }

                while (req->subs != NULL)
                {
                    tmp = req->subs;
                    req->subs = tmp->next_subs;
                    if (tmp->update != NULL)
                    {
                        free(tmp->update->location);
                        tmp->update->location = NULL;
                        free(tmp->update);
                        tmp->update = NULL;
                    }
                    free(tmp->location);
                    tmp->location = NULL;
                    free(tmp);
                    tmp = NULL;
                }

                if (pre_node == NULL)
                {
                    if (req->next_node == NULL)
                    {
                        free(req->need_distr_flag);
                        req->need_distr_flag = NULL;
                        free(req);
                        req = NULL;
                        ssd->request_queue = NULL;
                        ssd->request_tail = NULL;
                        ssd->request_queue_length--;
                    }
                    else
                    {
                        ssd->request_queue = req->next_node;
                        pre_node = req;
                        req = req->next_node;
                        free(pre_node->need_distr_flag);
                        pre_node->need_distr_flag = NULL;
                        free(pre_node);
                        pre_node = NULL;
                        ssd->request_queue_length--;
                    }
                }
                else
                {
                    if (req->next_node == NULL)
                    {
                        pre_node->next_node = NULL;
                        free(req->need_distr_flag);
                        req->need_distr_flag = NULL;
                        free(req);
                        req = NULL;
                        ssd->request_tail = pre_node;
                        ssd->request_queue_length--;
                    }
                    else
                    {
                        pre_node->next_node = req->next_node;
                        free(req->need_distr_flag);
                        req->need_distr_flag = NULL;
                        free(req);
                        req = pre_node->next_node;
                        ssd->request_queue_length--;
                    }
                }
            }
            else
            {
                pre_node = req;
                req = req->next_node;
            }
        }
    }
}

/*******************************************************************************
*statistic_output()函数主要是输出处理完一条请求后的相关处理信息。
*1，计算出每个plane的擦除次数即plane_erase和总的擦除次数即erase
*2，打印min_lsn，max_lsn，read_count，program_count等统计信息到文件outputfile中。
*3，打印相同的信息到文件statisticfile中
*******************************************************************************/
void statistic_output(struct ssd_info *ssd)
{
    unsigned int lpn_count = 0, i, j, k, m, p, erase = 0, plane_erase = 0;
    double gc_energy = 0.0;
#ifdef DEBUG
    printf("enter statistic_output,  current time:%lld\n", ssd->current_time);
#endif

    for (i = 0; i < ssd->parameter->channel_number; i++)
    {
        for (j = 0; j < ssd->parameter->chip_channel[0]; j++)
        {
            for (k = 0; k < ssd->parameter->die_chip; k++)
            {
                for (p = 0; p < ssd->parameter->plane_die; p++)
                {
                    plane_erase = 0;
                    for (m = 0; m < ssd->parameter->block_plane; m++)
                    {
                        if (ssd->channel_head[i].chip_head[j].die_head[k].plane_head[p].blk_head[m].erase_count > 0)
                        {
                            erase = erase + ssd->channel_head[i].chip_head[j].die_head[k].plane_head[p].blk_head[m].erase_count;
                            plane_erase += ssd->channel_head[i].chip_head[j].die_head[k].plane_head[p].blk_head[m].erase_count;
                        }
                    }
                    fprintf(ssd->outputfile, "the %d channel, %d chip, %d die, %d plane has : %13d erase operations\n", i, j, k, p, plane_erase);
                    fprintf(ssd->statisticfile, "the %d channel, %d chip, %d die, %d plane has : %13d erase operations\n", i, j, k, p, plane_erase);
                }
            }
        }
    }

    fprintf(ssd->outputfile, "\n");
    fprintf(ssd->outputfile, "\n");
    fprintf(ssd->outputfile, "---------------------------statistic data---------------------------\n");
    fprintf(ssd->outputfile, "min lsn: %13d\n", ssd->min_lsn);
    fprintf(ssd->outputfile, "max lsn: %13d\n", ssd->max_lsn);
    fprintf(ssd->outputfile, "read count: %13d\n", ssd->read_count);
    fprintf(ssd->outputfile, "program count: %13d", ssd->program_count);
    fprintf(ssd->outputfile, "                        include the flash write count leaded by read requests\n");
    fprintf(ssd->outputfile, "the read operation leaded by un-covered update count: %13d\n", ssd->update_read_count);
    fprintf(ssd->outputfile, "erase count: %13d\n", ssd->erase_count);
    fprintf(ssd->outputfile, "direct erase count: %13d\n", ssd->direct_erase_count);
    fprintf(ssd->outputfile, "copy back count: %13d\n", ssd->copy_back_count);
    fprintf(ssd->outputfile, "multi-plane program count: %13d\n", ssd->m_plane_prog_count);
    fprintf(ssd->outputfile, "multi-plane read count: %13d\n", ssd->m_plane_read_count);
    fprintf(ssd->outputfile, "interleave write count: %13d\n", ssd->interleave_count);
    fprintf(ssd->outputfile, "interleave read count: %13d\n", ssd->interleave_read_count);
    fprintf(ssd->outputfile, "interleave two plane and one program count: %13d\n", ssd->inter_mplane_prog_count);
    fprintf(ssd->outputfile, "interleave two plane count: %13d\n", ssd->inter_mplane_count);
    fprintf(ssd->outputfile, "gc copy back count: %13d\n", ssd->gc_copy_back);
    fprintf(ssd->outputfile, "write flash count: %13d\n", ssd->write_flash_count);
    //=================================================================
    fprintf(ssd->outputfile, "write LSB count: %13d\n", ssd->write_lsb_count);
    fprintf(ssd->outputfile, "write MSB count: %13d\n", ssd->write_msb_count);
    //=================================================================
    fprintf(ssd->outputfile, "interleave erase count: %13d\n", ssd->interleave_erase_count);
    fprintf(ssd->outputfile, "multiple plane erase count: %13d\n", ssd->mplane_erase_conut);
    fprintf(ssd->outputfile, "interleave multiple plane erase count: %13d\n", ssd->interleave_mplane_erase_count);
    fprintf(ssd->outputfile, "read request count: %13d\n", ssd->read_request_count);
    fprintf(ssd->outputfile, "write request count: %13d\n", ssd->write_request_count);
    fprintf(ssd->outputfile, "read request average size: %13f\n", ssd->ave_read_size);
    fprintf(ssd->outputfile, "write request average size: %13f\n", ssd->ave_write_size);
    fprintf(ssd->outputfile, "read request average response time: %lld\n", ssd->read_avg / ssd->read_request_count);
    fprintf(ssd->outputfile, "write request average response time: %lld\n", ssd->write_avg / ssd->write_request_count);
    fprintf(ssd->outputfile, "buffer read hits: %13d\n", ssd->dram->buffer->read_hit);
    fprintf(ssd->outputfile, "buffer read miss: %13d\n", ssd->dram->buffer->read_miss_hit);
    fprintf(ssd->outputfile, "buffer write hits: %13d\n", ssd->dram->buffer->write_hit);
    fprintf(ssd->outputfile, "buffer write miss: %13d\n", ssd->dram->buffer->write_miss_hit);
    fprintf(ssd->outputfile, "erase: %13d\n", erase);
    fprintf(ssd->outputfile, "sub_request_all: %13d, sub_request_success: %13d\n", ssd->sub_request_all, ssd->sub_request_success);
    fflush(ssd->outputfile);

    fclose(ssd->outputfile);

    fprintf(ssd->statisticfile, "\n");
    fprintf(ssd->statisticfile, "\n");
    fprintf(ssd->statisticfile, "---------------------------statistic data---------------------------\n");
    fprintf(ssd->statisticfile, "min lsn: %13d\n", ssd->min_lsn);
    fprintf(ssd->statisticfile, "max lsn: %13d\n", ssd->max_lsn);
    fprintf(ssd->statisticfile, "read count: %13d\n", ssd->read_count);
    fprintf(ssd->statisticfile, "program count: %13d", ssd->program_count);
    fprintf(ssd->statisticfile, "                        include the flash write count leaded by read requests\n");
    fprintf(ssd->statisticfile, "the read operation leaded by un-covered update count: %13d\n", ssd->update_read_count);
    fprintf(ssd->statisticfile, "erase count: %13d\n", ssd->erase_count);
    fprintf(ssd->statisticfile, "direct erase count: %13d\n", ssd->direct_erase_count);
    fprintf(ssd->statisticfile, "copy back count: %13d\n", ssd->copy_back_count);
    fprintf(ssd->statisticfile, "multi-plane program count: %13d\n", ssd->m_plane_prog_count);
    fprintf(ssd->statisticfile, "multi-plane read count: %13d\n", ssd->m_plane_read_count);
    fprintf(ssd->statisticfile, "interleave count: %13d\n", ssd->interleave_count);
    fprintf(ssd->statisticfile, "interleave read count: %13d\n", ssd->interleave_read_count);
    fprintf(ssd->statisticfile, "interleave two plane and one program count: %13d\n", ssd->inter_mplane_prog_count);
    fprintf(ssd->statisticfile, "interleave two plane count: %13d\n", ssd->inter_mplane_count);
    fprintf(ssd->statisticfile, "gc copy back count: %13d\n", ssd->gc_copy_back);
    fprintf(ssd->statisticfile, "write flash count: %13d\n", ssd->write_flash_count);
    //=================================================================
    fprintf(ssd->statisticfile, "write LSB count: %13d\n", ssd->write_lsb_count);
    fprintf(ssd->statisticfile, "write CSB count: %13d\n", ssd->write_csb_count);
    fprintf(ssd->statisticfile, "write MSB count: %13d\n", ssd->write_msb_count);
    //=================================================================
    fprintf(ssd->statisticfile, "waste page count: %13d\n", ssd->waste_page_count);
    fprintf(ssd->statisticfile, "interleave erase count: %13d\n", ssd->interleave_erase_count);
    fprintf(ssd->statisticfile, "multiple plane erase count: %13d\n", ssd->mplane_erase_conut);
    fprintf(ssd->statisticfile, "interleave multiple plane erase count: %13d\n", ssd->interleave_mplane_erase_count);
    fprintf(ssd->statisticfile, "read request count: %13d\n", ssd->read_request_count);
    fprintf(ssd->statisticfile, "write request count: %13d\n", ssd->write_request_count);
    fprintf(ssd->statisticfile, "read request average size: %13f\n", ssd->ave_read_size);
    fprintf(ssd->statisticfile, "write request average size: %13f\n", ssd->ave_write_size);
    fprintf(ssd->statisticfile, "read request average response time: %lld\n", ssd->read_avg / ssd->read_request_count);
    fprintf(ssd->statisticfile, "write request average response time: %lld\n", ssd->write_avg / ssd->write_request_count);
    if (ssd->write_request_count_l == 0)
    {
        fprintf(ssd->statisticfile, "large write request average response time: 0\n");
    }
    else
    {
        fprintf(ssd->statisticfile, "large write request average response time: %lld\n", ssd->write_avg_l / ssd->write_request_count_l);
    }
    fprintf(ssd->statisticfile, "buffer read hits: %13d\n", ssd->dram->buffer->read_hit);
    fprintf(ssd->statisticfile, "buffer read miss: %13d\n", ssd->dram->buffer->read_miss_hit);
    fprintf(ssd->statisticfile, "buffer write hits: %13d\n", ssd->dram->buffer->write_hit);
    fprintf(ssd->statisticfile, "buffer write miss: %13d\n", ssd->dram->buffer->write_miss_hit);
    fprintf(ssd->statisticfile, "erase: %13d\n", erase);
    fprintf(ssd->statisticfile, "sub_request_all: %13d, sub_request_success: %13d\n", ssd->sub_request_all, ssd->sub_request_success);
    fflush(ssd->statisticfile);

    fclose(ssd->statisticfile);
}

void statistic_output_easy(struct ssd_info *ssd, unsigned long completed_requests_num)
{
    unsigned int lpn_count = 0, i, j, k, m, erase = 0, plane_erase = 0;
    double gc_energy = 0.0;
#ifdef DEBUG
    //fprintf(ssd->debugfile,"enter statistic_output,  current time:%lld\n",ssd->current_time);
    //printf("enter statistic_output,  current time:%lld\n",ssd->current_time);
#endif
    unsigned long read_avg_lat, write_avg_lat, write_avg_lat_l;
    if (ssd->newest_read_request_count == 0)
    {
        read_avg_lat = 0;
    }
    else
    {
        read_avg_lat = ssd->newest_read_avg / ssd->newest_read_request_count;
    }
    if (ssd->newest_write_request_count == 0)
    {
        write_avg_lat = 0;
    }
    else
    {
        write_avg_lat = ssd->newest_write_avg / ssd->newest_write_request_count;
    }
    if (ssd->newest_write_request_num_l == 0)
    {
        write_avg_lat_l = 0;
    }
    else
    {
        write_avg_lat_l = ssd->newest_write_avg_l / ssd->newest_write_request_num_l;
    }
    fprintf(ssd->statisticfile, "%lld, %16lld, %13d, %13lld, %13lld, %13d, %13d, %13d, ", completed_requests_num, ssd->current_time, ssd->erase_count, read_avg_lat, write_avg_lat, ssd->newest_write_lsb_count, ssd->newest_write_csb_count, ssd->newest_write_msb_count);
    fprintf(ssd->statisticfile, "%13d, %13d, %13d, %13d, %13d, ", ssd->fast_gc_count, ssd->moved_page_count, ssd->free_lsb_count, ssd->newest_read_request_count, ssd->newest_write_request_count);
    fprintf(ssd->statisticfile, "%13d, %13d, %13d, %13d, ", ssd->newest_write_request_completed_with_same_type_pages, ssd->newest_req_with_lsb, ssd->newest_req_with_csb, ssd->newest_req_with_msb);
    fprintf(ssd->statisticfile, "% 13d, %13d, %13d, %13d, %13d, %13d\n", ssd->newest_write_request_num_l, ssd->newest_write_request_completed_with_same_type_pages_l, ssd->newest_req_with_lsb_l, ssd->newest_req_with_csb_l, ssd->newest_req_with_msb_l, write_avg_lat_l);

    //fprintf(ssd->statisticfile, "%13d, %13d, %13d\n", ssd->newest_write_request_completed_with_same_type_pages, ssd->newest_write_lsb_count, ssd->newest_write_msb_count);
    //fprintf(ssd->statisticfile,"\n\n");
    fflush(ssd->statisticfile);
}

/***********************************************************************************
*根据每一页的状态计算出每一需要处理的子页的数目，也就是一个子请求需要处理的子页的页数
************************************************************************************/
unsigned int size(unsigned int stored)
{
    unsigned int i, total = 0, mask = 0x80000000;

#ifdef DEBUG
    printf("enter size\n");
#endif
    for (i = 1; i <= 32; i++)
    {
        if (stored & mask)
            total++;
        stored <<= 1;
    }
#ifdef DEBUG
    printf("leave size\n");
#endif
    return total;
}

/*********************************************************
*transfer_size()函数的作用就是计算出子请求的需要处理的size
*函数中单独处理了first_lpn，last_lpn这两个特别情况，因为这
*两种情况下很有可能不是处理一整页而是处理一页的一部分，因
*为lsn有可能不是一页的第一个子页。
*********************************************************/
unsigned int transfer_size(struct ssd_info *ssd, int need_distribute, unsigned int lpn, struct request *req)
{
    unsigned int first_lpn, last_lpn, state, trans_size;
    unsigned int mask = 0, offset1 = 0, offset2 = 0;

    first_lpn = req->lsn / ssd->parameter->subpage_page;
    last_lpn = (req->lsn + req->size - 1) / ssd->parameter->subpage_page;

    mask = ~(0xffffffff << (ssd->parameter->subpage_page));
    state = mask;
    if (lpn == first_lpn)
    {
        offset1 = ssd->parameter->subpage_page - ((lpn + 1) * ssd->parameter->subpage_page - req->lsn);
        state = state & (0xffffffff << offset1);
    }
    if (lpn == last_lpn)
    {
        offset2 = ssd->parameter->subpage_page - ((lpn + 1) * ssd->parameter->subpage_page - (req->lsn + req->size));
        state = state & (~(0xffffffff << offset2));
    }

    trans_size = size(state & need_distribute);

    return trans_size;
}

/**********************************************************************************************************
*int64_t find_nearest_event(struct ssd_info *ssd)
*寻找所有子请求的最早到达的下个状态时间,首先看请求的下一个状态时间，如果请求的下个状态时间小于等于当前时间，
*说明请求被阻塞，需要查看channel或者对应die的下一状态时间。Int64是有符号 64 位整数数据类型，值类型表示值介于
*-2^63 ( -9,223,372,036,854,775,808)到2^63-1(+9,223,372,036,854,775,807 )之间的整数。存储空间占 8 字节。
*channel,die是事件向前推进的关键因素，三种情况可以使事件继续向前推进，channel，die分别回到idle状态，die中的
*读数据准备好了
***********************************************************************************************************/
int64_t find_nearest_event(struct ssd_info *ssd)
{
    unsigned int i, j;
    //首先函数会先定义三个初始参考变量值time、time1和time2，且全初始化为0x7fffffffffffffff）（无符号_int64类型的最大值）
    int64_t time = MAX_INT64;
    int64_t time1 = MAX_INT64;
    int64_t time2 = MAX_INT64;
    //随后进入一个for循环针对每一个channel进行一个所有事件的查询过程
    for (i = 0; i < ssd->parameter->channel_number; i++)
    { //若当前也就是第i个channel的下一状态为非空闲状态，则说明此时channel已经在空闲状态，那么就说明下一状态一定是工作状态，但是有两种可能的状态情况
        if (ssd->channel_head[i].next_state == CHANNEL_IDLE)
            if (time1 > ssd->channel_head[i].next_state_predict_time)
                if (ssd->channel_head[i].next_state_predict_time > ssd->current_time)
                    time1 = ssd->channel_head[i].next_state_predict_time;
        for (j = 0; j < ssd->parameter->chip_channel[i]; j++)
        {
            if ((ssd->channel_head[i].chip_head[j].next_state == CHIP_IDLE) || (ssd->channel_head[i].chip_head[j].next_state == CHIP_DATA_TRANSFER))
                if (time2 > ssd->channel_head[i].chip_head[j].next_state_predict_time)
                    if (ssd->channel_head[i].chip_head[j].next_state_predict_time > ssd->current_time)
                        time2 = ssd->channel_head[i].chip_head[j].next_state_predict_time;
        }
    }

    /*****************************************************************************************************
     *time为所有 A.下一状态为CHANNEL_IDLE且下一状态预计时间大于ssd当前时间的CHANNEL的下一状态预计时间
     *           B.下一状态为CHIP_IDLE且下一状态预计时间大于ssd当前时间的DIE的下一状态预计时间
     *             C.下一状态为CHIP_DATA_TRANSFER且下一状态预计时间大于ssd当前时间的DIE的下一状态预计时间
     *CHIP_DATA_TRANSFER读准备好状态，数据已从介质传到了register，下一状态是从register传往buffer中的最小值
     *注意可能都没有满足要求的time，这时time返回0x7fffffffffffffff 。
    *****************************************************************************************************/
    time = (time1 > time2) ? time2 : time1;
    return time;
}

/***********************************************
*free_all_node()函数的作用就是释放所有申请的节点
************************************************/
void free_all_node(struct ssd_info *ssd)
{
    unsigned int i, j, k, l, n;
    struct buffer_group *pt = NULL;
    struct direct_erase *erase_node = NULL;
    for (i = 0; i < ssd->parameter->channel_number; i++)
    {
        for (j = 0; j < ssd->parameter->chip_channel[0]; j++)
        {
            for (k = 0; k < ssd->parameter->die_chip; k++)
            {
                for (l = 0; l < ssd->parameter->plane_die; l++)
                {
                    for (n = 0; n < ssd->parameter->block_plane; n++)
                    {
                        free(ssd->channel_head[i].chip_head[j].die_head[k].plane_head[l].blk_head[n].page_head);
                        ssd->channel_head[i].chip_head[j].die_head[k].plane_head[l].blk_head[n].page_head = NULL;
                    }
                    free(ssd->channel_head[i].chip_head[j].die_head[k].plane_head[l].blk_head);
                    ssd->channel_head[i].chip_head[j].die_head[k].plane_head[l].blk_head = NULL;
                    while (ssd->channel_head[i].chip_head[j].die_head[k].plane_head[l].erase_node != NULL)
                    {
                        erase_node = ssd->channel_head[i].chip_head[j].die_head[k].plane_head[l].erase_node;
                        ssd->channel_head[i].chip_head[j].die_head[k].plane_head[l].erase_node = erase_node->next_node;
                        free(erase_node);
                        erase_node = NULL;
                    }
                }

                free(ssd->channel_head[i].chip_head[j].die_head[k].plane_head);
                ssd->channel_head[i].chip_head[j].die_head[k].plane_head = NULL;
            }
            free(ssd->channel_head[i].chip_head[j].die_head);
            ssd->channel_head[i].chip_head[j].die_head = NULL;
        }
        free(ssd->channel_head[i].chip_head);
        ssd->channel_head[i].chip_head = NULL;
    }
    free(ssd->channel_head);
    ssd->channel_head = NULL;

    avlTreeDestroy(ssd->dram->buffer);
    ssd->dram->buffer = NULL;

    free(ssd->dram->map->map_entry);
    ssd->dram->map->map_entry = NULL;
    free(ssd->dram->map);
    ssd->dram->map = NULL;
    free(ssd->dram);
    ssd->dram = NULL;
    free(ssd->parameter);
    ssd->parameter = NULL;

    free(ssd);
    ssd = NULL;
}

/*****************************************************************************
*make_aged()函数的作用就是模拟真实的用过一段时间的ssd，
*那么这个ssd的相应的参数就要改变，所以这个函数实质上就是对ssd中各个参数的赋值。
******************************************************************************/
struct ssd_info *make_aged(struct ssd_info *ssd)
{
    unsigned int i, j, k, l, m, n, ppn;
    int threshould, flag = 0;

    if (ssd->parameter->aged == 1) //aged=1表示要将这个ssd变成aged
    {
        //threshold表示一个plane中有多少页需要提前置为失效
        threshould = (int)(ssd->parameter->block_plane * ssd->parameter->page_block * ssd->parameter->aged_ratio); //plane里的总页数*aged的占比=aged的页数
        //用多层for循环对整个ssd的有必要的页置为失效
        for (i = 0; i < ssd->parameter->channel_number; i++)
            for (j = 0; j < ssd->parameter->chip_channel[i]; j++)
                for (k = 0; k < ssd->parameter->die_chip; k++)
                    for (l = 0; l < ssd->parameter->plane_die; l++) //到这一层，对每个plane里的需要置为失效的页置为失效
                    {
                        flag = 0;                                         //（宋：记录已经旧化了多少个页）
                        for (m = 0; m < ssd->parameter->block_plane; m++) //这其实是执行将plane里所有有需要的页置为无效（方式是一个页一个页的执行下去，需要进一步for循环细分）
                        {
                            if (flag >= threshould) //若该plane里没有要置为失效的页，则退出循环。旧化下一个plane
                            {
                                break;
                            }
                            for (n = 0; n < (ssd->parameter->page_block * ssd->parameter->aged_ratio + 1); n++) //若该plane里没有要置为失效的页，则一个页一个页的执行下去
                            {
                                ssd->channel_head[i].chip_head[j].die_head[k].plane_head[l].blk_head[m].page_head[n].valid_state = 0; //表示某一页失效，同时标记valid和free状态都为0
                                ssd->channel_head[i].chip_head[j].die_head[k].plane_head[l].blk_head[m].page_head[n].free_state = 0;  //表示某一页失效，同时标记valid和free状态都为0
                                ssd->channel_head[i].chip_head[j].die_head[k].plane_head[l].blk_head[m].page_head[n].lpn = 0;         //把valid_state free_state lpn都置为0表示页失效，检测的时候三项都检测，单独lpn=0可以是有效页
                                ssd->channel_head[i].chip_head[j].die_head[k].plane_head[l].blk_head[m].free_page_num--;
                                ssd->channel_head[i].chip_head[j].die_head[k].plane_head[l].blk_head[m].invalid_page_num++;
                                ssd->channel_head[i].chip_head[j].die_head[k].plane_head[l].blk_head[m].last_write_page++;
                                ssd->channel_head[i].chip_head[j].die_head[k].plane_head[l].free_page--;
                                flag++;
                                if (n % 3 == 0)
                                {
                                    ssd->channel_head[i].chip_head[j].die_head[k].plane_head[l].blk_head[m].last_write_lsb = n;
                                    ssd->free_lsb_count--;
                                    ssd->channel_head[i].chip_head[j].die_head[k].plane_head[l].blk_head[m].free_lsb_num--;
                                    //ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].free_page_num--;
                                    ssd->write_lsb_count++;
                                    ssd->newest_write_lsb_count++;
                                    ssd->channel_head[i].chip_head[j].die_head[k].plane_head[l].free_lsb_num--;
                                }
                                else if (n % 3 == 2)
                                {
                                    ssd->channel_head[i].chip_head[j].die_head[k].plane_head[l].blk_head[m].last_write_msb = n;
                                    ssd->channel_head[i].chip_head[j].die_head[k].plane_head[l].blk_head[m].free_msb_num--;
                                    //ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].free_page_num--;
                                    ssd->write_msb_count++;
                                    ssd->free_msb_count--;
                                    ssd->newest_write_msb_count++;
                                }
                                else
                                { //（n%3==1）
                                    ssd->channel_head[i].chip_head[j].die_head[k].plane_head[l].blk_head[m].last_write_csb = n;
                                    ssd->channel_head[i].chip_head[j].die_head[k].plane_head[l].blk_head[m].free_csb_num--;
                                    ssd->write_csb_count++;
                                    ssd->free_csb_count--;
                                    ssd->newest_write_csb_count++;
                                }
                                ppn = find_ppn(ssd, i, j, k, l, m, n); //根据失效页的物理地址返回失效页的物理页号（为什么要这一步骤）
                            }
                        }
                    }
    }
    else
    {
        return ssd;
    }

    return ssd;
}


int get_old_zwh(struct ssd_info *ssd)
{
    int cn_id, cp_id, di_id, pl_id;
    printf("Enter get_old_zwh.\n");
    for (cn_id = 0; cn_id < ssd->parameter->channel_number; cn_id++)
    {
        //printf("channel %d\n", cn_id);
        for (cp_id = 0; cp_id < ssd->parameter->chip_channel[0]; cp_id++)
        {
            //printf("chip %d\n", cp_id);
            for (di_id = 0; di_id < ssd->parameter->die_chip; di_id++)
            {
                //printf("die %d\n", di_id);
                for (pl_id = 0; pl_id < ssd->parameter->plane_die; pl_id++)
                {
                    //printf("channel %d, chip %d, die %d, plane %d: ", cn_id, cp_id, di_id, pl_id);
                    int active_block, ppn, lpn;
                    struct local *location;
                    lpn = 0;
                    while (ssd->channel_head[cn_id].chip_head[cp_id].die_head[di_id].plane_head[pl_id].free_page > (ssd->parameter->page_block * ssd->parameter->block_plane) * 0.3)
                    {
                        //if(cn_id==0&&cp_id==2&&di_id==0&&pl_id==0){
                        //    printf("cummulating....\n");
                        //    }
                        if (find_active_block(ssd, cn_id, cp_id, di_id, pl_id) == FAILURE)
                        {
                            printf("Wrong in get_old_zwh, find_active_block\n");
                            return 0;
                        }
                        active_block = ssd->channel_head[cn_id].chip_head[cp_id].die_head[di_id].plane_head[pl_id].active_block;
                        if (write_page(ssd, cn_id, cp_id, di_id, pl_id, active_block, &ppn) == ERROR)
                        {
                            return 0;
                        }
                        location = find_location(ssd, ppn);
                        ssd->program_count++;
                        ssd->channel_head[cn_id].program_count++;
                        ssd->channel_head[cn_id].chip_head[cp_id].program_count++;
                        ssd->channel_head[cn_id].chip_head[cp_id].die_head[di_id].plane_head[pl_id].blk_head[active_block].page_head[location->page].lpn = 0;
                        ssd->channel_head[cn_id].chip_head[cp_id].die_head[di_id].plane_head[pl_id].blk_head[active_block].page_head[location->page].valid_state = 0;
                        ssd->channel_head[cn_id].chip_head[cp_id].die_head[di_id].plane_head[pl_id].blk_head[active_block].page_head[location->page].free_state = 0;
                        ssd->channel_head[cn_id].chip_head[cp_id].die_head[di_id].plane_head[pl_id].blk_head[active_block].invalid_page_num++;
                        free(location);
                        location = NULL;
                    }
                    //printf("%d\n", ssd->channel_head[cn_id].chip_head[cp_id].die_head[di_id].plane_head[pl_id].free_page);
                }
            }
        }
    }
    printf("Exit get_old_zwh.\n");
}
/*********************************************************************************************
*no_buffer_distribute()函数是处理当ssd没有dram的时候，
*这时读写请求就不必再需要在buffer里面寻找，直接利用creat_sub_request()函数创建子请求，再处理。
*********************************************************************************************/
struct ssd_info *no_buffer_distribute(struct ssd_info *ssd)
{
    unsigned int lsn, lpn, last_lpn, first_lpn, complete_flag = 0, state;
    unsigned int flag = 0, flag1 = 1, active_region_flag = 0; //to indicate the lsn is hitted or not
    struct request *req = NULL;
    struct sub_request *sub = NULL, *sub_r = NULL, *update = NULL;
    struct local *loc = NULL;
    struct channel_info *p_ch = NULL;
    int i;

    unsigned int mask = 0;
    unsigned int offset1 = 0, offset2 = 0;
    unsigned int sub_size = 0;
    unsigned int sub_state = 0;

    ssd->dram->current_time = ssd->current_time;
    req = ssd->request_tail;
    lsn = req->lsn; //lsn是起始地址
    lpn = req->lsn / ssd->parameter->subpage_page;
    last_lpn = (req->lsn + req->size - 1) / ssd->parameter->subpage_page;
    first_lpn = req->lsn / ssd->parameter->subpage_page;

    if (req->operation == READ)
    {
        while (lpn <= last_lpn)
        {
            sub_state = (ssd->dram->map->map_entry[lpn].state & 0x7fffffff);
            sub_size = size(sub_state);
            sub = creat_sub_request(ssd, lpn, sub_size, sub_state, req, req->operation, 0);
            lpn++;
        }
    }
    else if (req->operation == WRITE)
    {
        int target_page_type;
        int random_num;
        random_num = rand() % 100;
        if (random_num < ssd->parameter->turbo_mode_factor)
        {
            target_page_type = TARGET_LSB;
        }
        else if (random_num < ssd->parameter->turbo_mode_factor_2)
        {
            target_page_type = TARGET_CSB;
        }
        else
        {
            target_page_type = TARGET_MSB;
        }
        while (lpn <= last_lpn)
        {
            if (ssd->parameter->subpage_page == 32)
            {
                mask = 0xffffffff;
            }
            else
            {
                mask = ~(0xffffffff << (ssd->parameter->subpage_page));
            }
            state = mask;
            //printf("initial state: %x\n", state);
            if (lpn == first_lpn)
            {
                offset1 = ssd->parameter->subpage_page - ((lpn + 1) * ssd->parameter->subpage_page - req->lsn);
                //printf("offset1: %d, ", offset1);
                state = state & (0xffffffff << offset1);
                //printf("state: %x\n", state);
            }
            if (lpn == last_lpn)
            {
                offset2 = ssd->parameter->subpage_page - ((lpn + 1) * ssd->parameter->subpage_page - (req->lsn + req->size));
                //printf("offset2: %d, ", offset2);
                if (offset2 != 32)
                {
                    state = state & (~(0xffffffff << offset2));
                }
                //printf("state: %x\n", state);
            }
            //printf("state: %x, ", state);
            sub_size = size(state);
            //printf("sub_size: %d\n", sub_size);
            sub = creat_sub_request(ssd, lpn, sub_size, state, req, req->operation, target_page_type);
            lpn++;
        }
    }

    return ssd;
}
