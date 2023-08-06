#pragma once

/*
 * The classes in this file are incomplete. As of March 26, 2023, they contain only enough implementation to make TMap:Set() work.
 *
 */

#pragma pack ( push, 0x4 )
class FBitReference
{
	DWORD& Data;
	DWORD Mask;
public:

	FBitReference(DWORD& data, DWORD mask)
		: Data(data)
		, Mask(mask)
	{}

	operator bool() const
	{
		return (Data & Mask) != 0;
	}
	void operator=(const bool NewValue)
	{
		if (NewValue)
		{
			Data |= Mask;
		}
		else
		{
			Data &= ~Mask;
		}
	}
};


class BitArray
{
	static const int NumInlineElements = 4;

	DWORD InlineData[NumInlineElements];
	DWORD* IndirectData;
	int NumBits;
	int MaxBits;

public:

	void Empty(int expectedNumBits = 0)
	{
		NumBits = 0;
		if (MaxBits != expectedNumBits)
		{
			MaxBits = expectedNumBits;
			Realloc(0);
		}
	}

	int AddItem(const bool Value)
	{
		const int index = NumBits;
		const bool shouldReallocate = (NumBits + 1) > MaxBits;

		NumBits++;

		if (shouldReallocate)
		{
			const UINT maxDWORDs = this->CalculateSlack((NumBits + 32 - 1) / 32);
			MaxBits = maxDWORDs * 32;
			Realloc(NumBits - 1);
		}

		(*this)(index) = Value;

		return index;
	}

	FBitReference operator()(int index)
	{
		return FBitReference(GetAllocation()[index / 32], 1 << (index & (32 - 1)));
	}

	const FBitReference operator()(int index) const
	{
		return FBitReference(GetAllocation()[index / 32], 1 << (index & (32 - 1)));
	}

private:

	DWORD* GetAllocation() const
	{
		return (DWORD*)(IndirectData ? IndirectData : InlineData);
	}

	void Realloc(int PreviousNumBits)
	{
		const int previousNumDWORDs = (PreviousNumBits + 32 - 1) / 32;
		const int maxDWORDs = (MaxBits + 32 - 1) / 32;

		ResizeAllocation(previousNumDWORDs, maxDWORDs, sizeof(DWORD));

		if (maxDWORDs)
		{
			memset(GetAllocation() + previousNumDWORDs, 0, (maxDWORDs - previousNumDWORDs) * sizeof(DWORD));
		}
	}

	void ResizeAllocation(int previousNumElements, int numElements, int numBytesPerElement)
	{
		const int previousNumBytes = previousNumElements * numBytesPerElement;
		
		if (numElements <= NumInlineElements)
		{
			if (IndirectData)
			{
				memcpy(InlineData, IndirectData, previousNumBytes);
				GMalloc.Free(IndirectData);
			}
		}
		else
		{
			if (IndirectData == nullptr)
			{
				IndirectData = (DWORD*)GMalloc.Malloc(numElements * numBytesPerElement);
				memcpy(IndirectData, InlineData, previousNumBytes);
			}
			else
			{
				IndirectData = (DWORD*)GMalloc.Realloc(IndirectData, numElements * numBytesPerElement);
			}
		}
	}

	int CalculateSlack(int numElements) const
	{
		return numElements <= NumInlineElements ? NumInlineElements : numElements;
	}
};

template<typename TElement>
class TSparseArray
{
	union ElementOrFreeListLink
	{
		TElement ElementData;
		int NextFreeIndex;
	};

	TArray<ElementOrFreeListLink> Data;
	BitArray AllocationFlags;
	int FirstFreeIndex;
	int NumFreeIndices;
public:
	int Num() const
	{
		return Data.Count - NumFreeIndices;
	}

	std::pair<void*, int> Add()
	{
		std::pair<void*, int> Result;
		if (NumFreeIndices > 0)
		{
			Result.second = FirstFreeIndex;
			FirstFreeIndex = GetData(FirstFreeIndex).NextFreeIndex;
			--NumFreeIndices;
		}
		else
		{
			Result.second = AddUninitializedSpaceToTArray(Data, 1);
			AllocationFlags.AddItem(TRUE);
		}

		// Compute the pointer to the new element's data.
		Result.first = &GetData(Result.second).ElementData;

		// Flag the element as allocated.
		AllocationFlags(Result.second) = TRUE;

		return Result;
	}

	class SparseArrayIterator
	{
		using iterator_category = std::forward_iterator_tag;
		using value_type = TElement;
		using difference_type = ptrdiff_t;
		using pointer = value_type*;
		using reference = value_type&;

		const TSparseArray& Array;
		int Index;
	public:
		
		SparseArrayIterator(const TSparseArray& arr) : Array(arr), Index(0) {}
		SparseArrayIterator& operator++()
		{
			do
			{
				++Index;
			}
			while (Index < Array.Data.Count && Array.IsAllocated(Index));
			return *this;
		}
		SparseArrayIterator operator++(int) { SparseArrayIterator retval = *this; ++(*this); return retval; }
		bool operator==(SparseArrayIterator other) const { return &Array == &other.Array && Index == other.Index; }
		bool operator!=(SparseArrayIterator other) const { return !(*this == other); }
		reference operator*() const { return Array.GetData(Index).ElementData; }

		int GetIndex() const { return Index; }

		operator bool() const
		{
			return Index < Array.Data.Count;
		}
	};

	TElement& operator()(int index)
	{
		return *(TElement*)&GetData(index).ElementData;
	}

	const TElement& operator()(int index) const
	{
		return *(TElement*)&GetData(index).ElementData;
	}

private:
	ElementOrFreeListLink& GetData(INT index) const
	{
		return Data.Data[index];
	}

	bool IsAllocated(int index) const
	{
		return AllocationFlags(index);
	}
};

class FSetElementId
{
	int Index;
public:
	FSetElementId() : Index(-1){}

	FSetElementId(int index) : Index(index) {}

	bool IsValidId() const
	{
		return Index != -1;
	}

	friend bool operator==(const FSetElementId& a, const FSetElementId& b)
	{
		return a.Index == b.Index;
	}

	operator int() const
	{
		return Index;
	}
};

template<typename TElement, bool CAllowDuplicateKeys = false>
struct DefaultKeyFuncs
{
	typedef typename TElement KeyType;
	enum { AllowDuplicateKeys = CAllowDuplicateKeys };

	static KeyType& GetKey(TElement& element)
	{
		return element;
	}

	static bool Matches(const KeyType& a, const KeyType& b)
	{
		return a == b;
	}

	static DWORD GetKeyHash(const KeyType element)
	{
		return GetTypeHash(element);
	}
};

unsigned int bit_ceil(unsigned int value)
{
	--value;
	value |= value >> 1;
	value |= value >> 2;
	value |= value >> 4;
	value |= value >> 8;
	value |= value >> 16;
	return value + 1;
}

template<typename TElement, typename TKeyFuncs = DefaultKeyFuncs<TElement>>
class TSet
{
	typedef typename TKeyFuncs::KeyType KeyType;
	typedef typename TElement ElementType;

	class FElement
	{
	public:
		ElementType Value;
		mutable FSetElementId HashNextId;
		mutable int HashIndex;

		FElement(ElementType& value) : Value(value) {};
	};

	TSparseArray<FElement> Elements;
	mutable FSetElementId InlineHash;
	mutable FSetElementId* Hash;
	mutable int HashSize;
public:
	FSetElementId Add(ElementType& element)
	{
		FSetElementId id = FindId(TKeyFuncs::GetKey(element));
		if (!id.IsValidId())
		{
			auto elementAllocation = Elements.Add();
			id = FSetElementId(elementAllocation.second);
			FElement& fElement = *new(elementAllocation.first) FElement(element);
			
			if (!ConditionalRehash(Elements.Num()))
			{
				HashElement(id, fElement);
			}
		}
		else
		{
			ElementType& a = Elements(id).Value;
			a.~ElementType();
			new (&a) ElementType(element);
		}
		return id;
	}

	ElementType& operator()(FSetElementId id)
	{
		return Elements(id).Value;
	}

	ElementType* Find(KeyType key)
	{
		FSetElementId elementId = FindId(key);
		if (elementId.IsValidId())
		{
			return &Elements(elementId).Value;
		}
		else
		{
			return nullptr;
		}
	}

	const ElementType* Find(KeyType key) const
	{
		FSetElementId elementId = FindId(key);
		if (elementId.IsValidId())
		{
			return &Elements(elementId).Value;
		}
		else
		{
			return nullptr;
		}
	}

private:

	int GetNumberOfHashBuckets(int numHashedElements) const
	{
		const unsigned int elementsPerBucket = 2;
		const unsigned int baseNumberOfBuckets = 8;
		const unsigned int minNumberOfHashedElements = 4;

		if (numHashedElements >= minNumberOfHashedElements)
		{
			return (int)bit_ceil((unsigned int)numHashedElements / elementsPerBucket + baseNumberOfBuckets);
		}
		return 1;
	}

	FSetElementId& GetTypedHash(INT hashIndex) const
	{
		return GetHash()[hashIndex & (HashSize - 1)];
	}

	void HashElement(FSetElementId elementId, const FElement& element) const
	{
		element.HashIndex = TKeyFuncs::GetKeyHash(TKeyFuncs::GetKey(element.Value)) & (HashSize - 1);
		
		element.HashNextId = GetTypedHash(element.HashIndex);
		GetTypedHash(element.HashIndex) = elementId;
	}

	void Rehash() const
	{
		ResizeHash(0, 0);

		if (HashSize)
		{
			ResizeHash(0, HashSize);
			for (int i = 0; i < HashSize; i++)
			{
				GetTypedHash(i) = FSetElementId();
			}

			for (auto it = TSparseArray<FElement>::SparseArrayIterator(Elements); it; ++it)
			{
				HashElement(FSetElementId(it.GetIndex()), *it);
			}
		}
	}

	FSetElementId* GetHash() const
	{
		return Hash ? Hash : &InlineHash;
	}

	void ResizeHash(int previousNumElements, int numElements) const
	{
		const int previousNumBytes = previousNumElements * sizeof(FSetElementId);

		if (numElements <= 1)
		{
			if (Hash)
			{
				memcpy(&InlineHash, Hash, previousNumBytes);
				GMalloc.Free(Hash);
			}
		}
		else
		{
			if (Hash == nullptr)
			{
				Hash = (FSetElementId*)GMalloc.Malloc(numElements * sizeof(FSetElementId));
				memcpy(Hash, &InlineHash, previousNumBytes);
			}
			else
			{
				Hash = (FSetElementId*)GMalloc.Realloc(Hash, numElements * sizeof(FSetElementId));
			}
		}
	}

	bool ConditionalRehash(int numHashedElements) const
	{
		const int desiredHashSize = GetNumberOfHashBuckets(numHashedElements);
		
		if (numHashedElements > 0 && (!HashSize || HashSize < desiredHashSize))
		{
			HashSize = desiredHashSize;
			Rehash();
			return true;
		}
		else
		{
			return false;
		}
	}

	FSetElementId FindId(KeyType key) const
	{
		if (HashSize)
		{
			for (FSetElementId elementId = GetTypedHash(TKeyFuncs::GetKeyHash(key)); 
				elementId.IsValidId();
				elementId = Elements(elementId).HashNextId)
			{
				if (TKeyFuncs::Matches(TKeyFuncs::GetKey(Elements(elementId).Value), key))
				{
					return elementId;
				}
			}
		}
		return FSetElementId();
	}
};

template<typename TKey, typename TValue>
class TMap
{
	class FPair
	{
	public:
		TKey Key;
		TValue Value;

		FPair(TKey& key, TValue& value) : Key(key), Value(value) {}
		FPair(const FPair& other) : Key(other.Key), Value(other.Value) {}
	};

	struct KeyFuncs
	{
		typedef typename TKey KeyType;

		enum { AllowDuplicateKeys = false };

		static const KeyType& GetKey(const FPair& element)
		{
			return element.Key;
		}

		static bool Matches(const KeyType& a, const KeyType& b)
		{
			return a == b;
		}

		static DWORD GetKeyHash(const KeyType element)
		{
			return GetTypeHash(element);
		}
	};

	TSet<FPair, KeyFuncs> Pairs;
public:

	TValue& Set(TKey& key, TValue& value)
	{
		FPair pair(key, value);
		const FSetElementId id = Pairs.Add(pair);
		return Pairs(id).Value;
	}

	TValue* Find(const TKey* key)
	{
		FPair* pair = Pairs.Find(key);
		return pair ? &pair->Value : nullptr;
	}

	const TValue* Find(const TKey* key) const
	{
		FPair* pair = Pairs.Find(key);
		return pair ? &pair->Value : nullptr;
	}
};

#pragma pack ( pop )
