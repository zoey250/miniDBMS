
#ifndef TEMPL_LINKED_LIST_H
#define TEMPL_LINKED_LIST_H

//链表模板类的定义
template <class T>
class LinkList
{
public:
    //构造函数
    LinkList ( );
    LinkList ( const LinkList<T> & sourcell );

    //析构函数
    ~LinkList();

    //运算符
    //赋值
    void operator = ( const LinkList<T> & sourcell );

    //等价
    Boolean operator == ( const LinkList<T> & rhs ) const;

    //相加
    LinkList<T> operator+(const LinkList<T> &sourcell) const;
    LinkList<T> operator+(const T &element) const;
    //加等
    void operator+=(const LinkList<T> &sourcell);
    void operator+=(const T &element);

    //复制LinkList并将其转换为数组
    operator T * ();

    //追加新项
    void Append ( const T & item );
    void Append ( const LinkList<T> & sourcell );

    //删除当前项
    void Delete( int index );

    //从列表中删除所有项
    void Erase();

    //获取当前项。 索引的有效范围是：0 ..（1-GetLength（））
    T* Get( int index );

    //获取下一个元素
    T* operator[](int index);

    //获取链表的长度
    int  GetLength() const;

protected:
    //链表中的内部节点，包含我们要存储的数据以及指向上一个和下一个的指针
    struct InternalNode {
          T Data;
          InternalNode * next;
          InternalNode * previous;
    };

    //清单中的项目数
    int iLength;
    //第一项
    InternalNode *pnHead;
    //最后一项
    InternalNode *pnTail;
    //当前项目
    InternalNode *pnLastRef;
    //最后引用哪个元素
    int iLastRef;

    //为类的Private成员设置初始值
    void SetNull();
};


//设置（或重置）的私有成员的初始值
template <class T>
inline void LinkList<T>::SetNull()
{
    pnHead = NULL;
    pnTail = NULL;
    pnLastRef = NULL;
    iLength = 0;
    iLastRef = -1;
}

//构造函数
template <class T>
inline LinkList<T>::LinkList ()
{
    SetNull();
}


template <class T>
LinkList<T>::LinkList ( const LinkList<T> & sourcell )
{
    //初始化新列表
    SetNull();

    //复制所有传入列表的成员
    if (sourcell.iLength == 0)
        return;

    InternalNode *n = sourcell.pnHead;

    while (n != NULL)
    {
        Append(n->Data);
        n = n->next;
    }
    pnLastRef = pnHead;
}

//析构函数
template <class T>
inline LinkList<T>::~LinkList()
{
    Erase();
}

//赋值运算符
template <class T>
void LinkList<T>::operator = ( const LinkList<T> & sourcell )
{
    //首先清除原始清单
    Erase();

    //之后复制传入的列表
    InternalNode *pnTemp = sourcell.pnHead;

    while (pnTemp != NULL)
    {
        Append(pnTemp->Data);
        pnTemp = pnTemp->next;
    }

    pnLastRef = NULL;
    iLastRef = -1;
}

//等价运算符
template <class T>
Boolean LinkList<T>::operator == ( const LinkList<T> & rhs ) const
{
    if (iLength != rhs.iLength)
        return (FALSE);

    InternalNode *pnLhs = this->pnHead;
    InternalNode *pnRhs = rhs.pnHead;

    while (pnLhs != NULL && pnRhs != NULL)
    {
        //模板设置的数据类型T定义相等性
        if (!(pnLhs->Data == pnRhs->Data))
            return FALSE;
        pnLhs = pnLhs->next;
        pnRhs = pnRhs->next;
    }

    if (pnLhs==NULL && pnRhs==NULL)
        return TRUE;
    else
        return FALSE;
}


//加和加等
template <class T>
inline LinkList<T>
LinkList<T>::operator+(const LinkList<T> &sourcell) const
{
    LinkList<T> pTempLL(*this);
    pTempLL += sourcell;
    return pTempLL;
}


template <class T>
inline LinkList<T>
LinkList<T>::operator+(const T &element) const
{
    LinkList<T> pTempLL(*this);
    pTempLL += element;
    return pTempLL;
}


template <class T>
void
LinkList<T>::operator+=(const LinkList<T> &list)
{
    const InternalNode *pnTemp;
    const int iLength = list.iLength;
    int i;

    //必须使用大小作为停止条件，以防* this == 列表
    for (pnTemp = list.pnHead, i=0; i < iLength; pnTemp = pnTemp->next, i++)
        *this += pnTemp->Data;
}


template <class T>
void
LinkList<T>::operator+=(const T &element)
{
    InternalNode *pnNew = new InternalNode;
    pnNew->next = NULL;
    pnNew->Data = element;
    if (iLength++ == 0) {
        pnHead = pnNew;
        pnNew->previous = NULL;
    }
    else {
        pnTail->next = pnNew;
        pnNew->previous = pnTail;
    }
    pnTail = pnNew;
}


//转换为数组运算符（返回列表的副本）
template <class T>
LinkList<T>::operator T * ()
{
    if (iLength == 0)
        return NULL;

    T *pResult = new T[iLength];

    InternalNode *pnCur = pnHead;
    T *pnCopy = pResult;

    while (pnCur != NULL)
    {
        *pnCopy = pnCur->Data;
        ++pnCopy;
        pnCur = pnCur->next;
    }

    // Note:  This is a copy of the list and the caller must delete it when
    // done.
    return pResult;
}

//将新项目追加到链接列表的末尾
template <class T>
inline void LinkList<T>::Append ( const T & item )
{
    InternalNode *pnNew = new InternalNode;

    pnNew->Data = item;
    pnNew->next = NULL;
    pnNew->previous = pnTail;

    //如果是第一个，则将head设置为此元素
    if (iLength == 0)
    {
        pnHead = pnNew;
        pnTail = pnNew;
        pnLastRef = pnNew;
    }
    else
    {
        //将尾巴设置为此新元素
        pnTail->next = pnNew;
        pnTail = pnNew;
    }

    ++iLength;
}


template <class T>
void LinkList<T>::Append ( const LinkList<T> & sourcell )
{
    const InternalNode *pnCur = sourcell.pnHead;

    while (pnCur != NULL)
    {
        Append(pnCur->Data);
        pnCur = pnCur->next;
    }
}

//删除指定的元素
template <class T>
inline void LinkList<T>::Delete(int which)
{
    if (which>iLength || which == 0)
        return;

    InternalNode *pnDeleteMe = pnHead;

    for (int i=1; i<which; i++)
        pnDeleteMe = pnDeleteMe->next;

    if (pnDeleteMe == pnHead)
    {
        if (pnDeleteMe->next == NULL)
        {
            delete pnDeleteMe;
            SetNull();
        }
        else
        {
            pnHead = pnDeleteMe->next;
            pnHead->previous = NULL;
            delete pnDeleteMe;
            pnLastRef = pnHead;
        }
    }
    else
    {
        if (pnDeleteMe == pnTail)
        {
            if (pnDeleteMe->previous == NULL)
            {
                delete pnDeleteMe;
                SetNull();
            }
            else
            {
                pnTail = pnDeleteMe->previous;
                pnTail->next = NULL;
                delete pnDeleteMe;
                pnLastRef = pnTail;
            }
        }
        else
        {
            pnLastRef = pnDeleteMe->next;
            pnDeleteMe->previous->next = pnDeleteMe->next;
            pnDeleteMe->next->previous = pnDeleteMe->previous;
            delete pnDeleteMe;
        }
    }

    if (iLength!=0)
        --iLength;
}

template <class T>
inline T* LinkList<T>::operator[](int index)
{
    return (Get(index));
}

//从列表中删除所有项目
template <class T>
inline void LinkList<T>::Erase()
{
    pnLastRef = pnHead;

    while (pnLastRef != NULL)
    {
        pnHead = pnLastRef->next;
        delete pnLastRef;
        pnLastRef = pnHead;
    }
    SetNull();
}

//获取指定的项目
template <class T>
inline T* LinkList<T>::Get(int index)
{
    //从中开始搜索的位置
    int iCur;
    //从中开始搜索的节点
    InternalNode *pnTemp;
    //要求相对于上一个参考的位置
    int iRelToMiddle;

    //确保该项目在范围内
    if (index < 0 || index >= iLength)
        return NULL;

    //将索引设为基数1使此过程更加容易。
    index++;

    if (iLastRef==-1)
        if (index < (iLength-index)) {
            iCur = 1;
            pnTemp = pnHead;
        } else {
            iCur = iLength;
            pnTemp = pnTail;
        }
    else
        {
            if (index < iLastRef)
                iRelToMiddle = iLastRef - index;
            else
                iRelToMiddle = index - iLastRef;

            if (index < iRelToMiddle) {
                //头部最接近请求的元素
                iCur = 1;
                pnTemp = pnHead;
            }
            else
                if (iRelToMiddle < (iLength - index)) {
                    iCur = iLastRef;
                    pnTemp = pnLastRef;
                } else {
                    iCur = iLength;
                    pnTemp = pnTail;
                }
        }

    //从决定的第一个元素开始找到所需的元素
    while (iCur != index)
        if (iCur < index) {
            iCur++;
            pnTemp = pnTemp->next;
        } else {
            iCur--;
            pnTemp = pnTemp->previous;
        }

    iLastRef = index;
    pnLastRef = pnTemp;

    return &(pnLastRef->Data);
}

/****************************************************************************/

template <class T>
inline int LinkList<T>::GetLength() const
{
    return iLength;
}

#endif
