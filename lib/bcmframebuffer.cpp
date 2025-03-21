//
// bcmframebuffer.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2014-2020  R. Stange <rsta2@o2online.de>
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
#include <circle/bcmframebuffer.h>
#include <circle/interrupt.h>
#include <circle/util.h>
#include <circle/koptions.h>
#include <circle/atomic.h>

#define PTR_ADD(type, ptr, items, bytes)	((type) ((uintptr) (ptr) + (bytes)) + (items))

const TBcmFrameBufferInitTags CBcmFrameBuffer::s_InitTags =
{
	{{PROPTAG_SET_PHYS_WIDTH_HEIGHT, 8, 8}},
	{{PROPTAG_SET_VIRT_WIDTH_HEIGHT, 8, 8}},
	{{PROPTAG_SET_DEPTH,		 4, 4}},
	{{PROPTAG_SET_VIRTUAL_OFFSET,	 8, 8}, 0, 0},
	{{PROPTAG_ALLOCATE_BUFFER,	 8, 4}, {0}},
	{{PROPTAG_GET_PITCH,		 4, 0}}
};

#if RASPPI >= 4

unsigned CBcmFrameBuffer::s_nDisplays = 0;

unsigned CBcmFrameBuffer::s_nCurrentDisplay = (unsigned) -1;

#endif

CBcmFrameBuffer::CBcmFrameBuffer (unsigned nWidth, unsigned nHeight, unsigned nDepth,
				  unsigned nVirtualWidth, unsigned nVirtualHeight,
				  unsigned nDisplay, boolean bDoubleBuffered)
:	CDisplay (nDepth == 8 ? I8 : (nDepth == 16 ? RGB565 : ARGB8888)),
	m_nWidth (nWidth),
	m_nHeight (nHeight),
	m_nVirtualWidth (nVirtualWidth),
	m_nVirtualHeight (nVirtualHeight),
	m_nDepth (nDepth),
	m_nDisplay (nDisplay),
	m_nBufferPtr (0),
	m_nBufferSize (0),
	m_nPitch (0),
	m_pTagSetPalette (0)
#ifdef SCREEN_DMA_BURST_LENGTH
	, m_DMAChannel (DMA_CHANNEL_NORMAL, CInterruptSystem::Get ()),
	m_nDMAInUse (0)
#endif
{
	if (m_nDisplay >= GetNumDisplays ())
	{
		return;
	}

	if (   m_nWidth  == 0
	    || m_nHeight == 0)
	{
		SetDisplay ();

		// detect optimal display size (if not configured)
		CBcmPropertyTags Tags;
		TPropertyTagDisplayDimensions Dimensions;
		if (Tags.GetTag (PROPTAG_GET_DISPLAY_DIMENSIONS, &Dimensions, sizeof Dimensions))
		{
			m_nWidth  = Dimensions.nWidth;
			m_nHeight = Dimensions.nHeight;

			if (   m_nWidth  < 640
			    || m_nWidth  > 4096
			    || m_nHeight < 480
			    || m_nHeight > 2160)
			{
				m_nWidth  = 640;
				m_nHeight = 480;
			}
		}
		else
		{
			m_nWidth  = 640;
			m_nHeight = 480;
		}
	}

	if (   m_nVirtualWidth  == 0
	    || m_nVirtualHeight == 0)
	{
		m_nVirtualWidth  = m_nWidth;
		m_nVirtualHeight = m_nHeight * (1 + bDoubleBuffered);
	}

	if (m_nDepth <= 8)
	{
		m_pTagSetPalette = (TPropertyTagSetPalette *) new u8 [sizeof (TPropertyTagSetPalette)
								      + PALETTE_ENTRIES * sizeof (u32)];

		memset (m_pTagSetPalette->Palette, 0, PALETTE_ENTRIES * sizeof (u32));	// clear palette
	}

	memcpy (&m_InitTags, &s_InitTags, sizeof s_InitTags);

	m_InitTags.SetPhysWidthHeight.nWidth  = m_nWidth;
	m_InitTags.SetPhysWidthHeight.nHeight = m_nHeight;
	m_InitTags.SetVirtWidthHeight.nWidth  = m_nVirtualWidth;
	m_InitTags.SetVirtWidthHeight.nHeight = m_nVirtualHeight;
	m_InitTags.SetDepth.nValue            = m_nDepth;
}

CBcmFrameBuffer::~CBcmFrameBuffer (void)
{
	delete m_pTagSetPalette;
	m_pTagSetPalette = 0;
}

void CBcmFrameBuffer::SetPalette (u8 nIndex, u16 nRGB565)
{
	if (m_nDepth <= 8)
	{
		u32 nRGBA;
		nRGBA  = (u32) (nRGB565 >> 11 & 0x1F) << (0+3);		// red
		nRGBA |= (u32) (nRGB565 >> 5  & 0x3F) << (8+2);		// green
		nRGBA |= (u32) (nRGB565       & 0x1F) << (16+3);	// blue
		nRGBA |=        0xFF                  << 24;		// alpha

		m_pTagSetPalette->Palette[nIndex] = nRGBA;
	}
}

void CBcmFrameBuffer::SetPalette32 (u8 nIndex, u32 nRGBA)
{
	if (m_nDepth <= 8)
	{
		m_pTagSetPalette->Palette[nIndex] = nRGBA;
	}
}

boolean CBcmFrameBuffer::Initialize (void)
{
	if (m_nDisplay >= GetNumDisplays ())
	{
		return FALSE;
	}

	SetDisplay ();

	CBcmPropertyTags Tags;
	if (!Tags.GetTags (&m_InitTags, sizeof m_InitTags))
	{
		return FALSE;
	}

	if (   m_InitTags.SetPhysWidthHeight.nWidth         == 0
	    || m_InitTags.SetPhysWidthHeight.nHeight        == 0
	    || m_InitTags.SetVirtWidthHeight.nWidth         == 0
	    || m_InitTags.SetVirtWidthHeight.nHeight        == 0
	    || m_InitTags.SetDepth.nValue                   == 0
	    || m_InitTags.AllocateBuffer.nBufferBaseAddress == 0)
	{
		return FALSE;
	}

	m_nBufferPtr  = m_InitTags.AllocateBuffer.nBufferBaseAddress & 0x3FFFFFFF;
	m_nBufferSize = m_InitTags.AllocateBuffer.nBufferSize;
	m_nPitch      = m_InitTags.GetPitch.nValue;

	boolean bOK = UpdatePalette();
	if (bOK)
	{
		unsigned nBrightness = 0;

		CKernelOptions* pOptions = CKernelOptions::Get();
		if (pOptions)
		{
			nBrightness = pOptions->GetBacklight();
		}

		if (nBrightness != 0)
		{
			bOK = SetBacklightBrightness (nBrightness);
		}
	}
	return bOK;
}

void CBcmFrameBuffer::SetPixel (unsigned nPosX, unsigned nPosY, TRawColor nColor)
{
	switch (m_nDepth)
	{
	case 8: {
			auto p = PTR_ADD (u8 *, m_nBufferPtr, nPosX, nPosY*m_nPitch);
			*p = static_cast<u8> (nColor);
		} break;

	case 16: {
			auto p = PTR_ADD (u16 *, m_nBufferPtr, nPosX, nPosY*m_nPitch);
			*p = static_cast<u16> (nColor);
		} break;

	case 32: {
			auto p = PTR_ADD (u32 *, m_nBufferPtr, nPosX, nPosY*m_nPitch);
			*p = static_cast<u32> (nColor);
		} break;
	}
}

void CBcmFrameBuffer::SetArea (const TArea &rArea, const void *pPixels,
			       TAreaCompletionRoutine *pRoutine, void *pParam)
{
#ifdef SCREEN_DMA_BURST_LENGTH
	while (AtomicCompareExchange (&m_nDMAInUse, 0, 1))
	{
		// TODO: Yield with NO_BUSY_WAIT, when on core 0 in TASK_LEVEL
	}

	u32 x1 = rArea.x1;
	u32 x2 = rArea.x2;
	u32 y1 = rArea.y1;
	u32 y2 = rArea.y2;

	void *pDestination = (void *) (uintptr) (m_nBufferPtr + y1*m_nPitch + x1*m_nDepth/8);

	size_t ulBlockLength = (x2-x1+1) * m_nDepth/8;

	m_DMAChannel.SetupMemCopy2D (pDestination, pPixels, ulBlockLength, y2-y1+1,
				     m_nPitch-ulBlockLength, SCREEN_DMA_BURST_LENGTH);

	if (pRoutine)
	{
		m_pCompletionRoutine = pRoutine;
		m_pCompletionParam = pParam;

		m_DMAChannel.SetCompletionRoutine (DMACompletionRoutine, this);
		m_DMAChannel.Start ();
	}
	else
	{
		m_DMAChannel.Start ();
		m_DMAChannel.Wait ();

		AtomicSet (&m_nDMAInUse, 0);
	}
#else
	size_t ulWidth = rArea.x2 - rArea.x1 + 1;
	size_t ulBlockLength = ulWidth * m_nDepth/8;

	switch (m_nDepth)
	{
	case 8: {
			auto p = PTR_ADD (u8 *, m_nBufferPtr, rArea.x1, rArea.y1 * m_nPitch);
			auto q = static_cast<const u8 *> (pPixels);

			for (unsigned y = rArea.y1; y <= rArea.y2; y++, q += ulWidth)
			{
				memcpy (p, q, ulBlockLength);

				p = PTR_ADD (u8 *, p, 0, m_nPitch);
			}
		} break;

	case 16: {
			auto p = PTR_ADD (u16 *, m_nBufferPtr, rArea.x1, rArea.y1 * m_nPitch);
			auto q = static_cast<const u16 *> (pPixels);

			for (unsigned y = rArea.y1; y <= rArea.y2; y++, q += ulWidth)
			{
				memcpy (p, q, ulBlockLength);

				p = PTR_ADD (u16 *, p, 0, m_nPitch);
			}
		} break;

	case 32: {
			auto p = PTR_ADD (u32 *, m_nBufferPtr, rArea.x1, rArea.y1 * m_nPitch);
			auto q = static_cast<const u32 *> (pPixels);

			for (unsigned y = rArea.y1; y <= rArea.y2; y++, q += ulWidth)
			{
				memcpy (p, q, ulBlockLength);

				p = PTR_ADD (u32 *, p, 0, m_nPitch);
			}
		} break;
	}

	if (pRoutine)
	{
		(*pRoutine) (pParam);
	}
#endif
}

#ifdef SCREEN_DMA_BURST_LENGTH

void CBcmFrameBuffer::DMACompletionRoutine (unsigned nChannel, unsigned nBuffer,
					    boolean bStatus, void *pParam)
{
	CBcmFrameBuffer *pThis = static_cast<CBcmFrameBuffer *> (pParam);

	AtomicSet (&pThis->m_nDMAInUse, 0);

	(*pThis->m_pCompletionRoutine) (pThis->m_pCompletionParam);
}

#endif

unsigned CBcmFrameBuffer::GetWidth (void) const
{
	return m_nWidth;
}

unsigned CBcmFrameBuffer::GetHeight (void) const
{
	return m_nHeight;
}

unsigned CBcmFrameBuffer::GetVirtWidth (void) const
{
	return m_nVirtualWidth;
}

unsigned CBcmFrameBuffer::GetVirtHeight (void) const
{
	return m_nVirtualHeight;
}

u32 CBcmFrameBuffer::GetPitch (void) const
{
	return m_nPitch;
}

u32 CBcmFrameBuffer::GetDepth (void) const
{
	return m_nDepth;
}

u32 CBcmFrameBuffer::GetBuffer (void) const
{
	return m_nBufferPtr;
}

u32 CBcmFrameBuffer::GetSize (void) const
{
	return m_nBufferSize;
}

boolean CBcmFrameBuffer::UpdatePalette (void)
{
	if (m_nDepth <= 8)
	{
		SetDisplay ();

		m_pTagSetPalette->nOffset = 0;
		m_pTagSetPalette->nLength = PALETTE_ENTRIES;

		CBcmPropertyTags Tags;
		if (!Tags.GetTag (PROPTAG_SET_PALETTE, m_pTagSetPalette,
				  sizeof *m_pTagSetPalette + PALETTE_ENTRIES*sizeof (u32),
				  (2+PALETTE_ENTRIES) * sizeof (u32)))
		{
			return FALSE;
		}

		if (m_pTagSetPalette->nResult != SET_PALETTE_VALID)
		{
			return FALSE;
		}
	}

	return TRUE;
}

boolean CBcmFrameBuffer::SetVirtualOffset (u32 nOffsetX, u32 nOffsetY)
{
	SetDisplay ();

	CBcmPropertyTags Tags;
	TPropertyTagVirtualOffset VirtualOffset;
	VirtualOffset.nOffsetX = nOffsetX;
	VirtualOffset.nOffsetY = nOffsetY;
	if (   !Tags.GetTag (PROPTAG_SET_VIRTUAL_OFFSET, &VirtualOffset, sizeof VirtualOffset, 8)
	    || VirtualOffset.nOffsetX != nOffsetX
	    || VirtualOffset.nOffsetY != nOffsetY)
	{
		return FALSE;
	}

	return TRUE;
}

boolean CBcmFrameBuffer::WaitForVerticalSync (void)
{
	SetDisplay ();

	CBcmPropertyTags Tags;
	TPropertyTagSimple Dummy;
	return Tags.GetTag (PROPTAG_WAIT_FOR_VSYNC, &Dummy, sizeof Dummy);
}

boolean CBcmFrameBuffer::SetBacklightBrightness(unsigned nBrightness)
{
	SetDisplay ();

	CBcmPropertyTags Tags;
	TPropertyTagSimple TagBrightness;
	TagBrightness.nValue = nBrightness;
	return Tags.GetTag (PROPTAG_SET_BACKLIGHT, &TagBrightness, sizeof TagBrightness, 4);
}

void CBcmFrameBuffer::SetDisplay (void)
{
#if RASPPI >= 4
	if (m_nDisplay == s_nCurrentDisplay)
	{
		return;
	}

	CBcmPropertyTags Tags;
	TPropertyTagSimple TagDisplayNum;
	TagDisplayNum.nValue = m_nDisplay;
	if (Tags.GetTag (PROPTAG_SET_DISPLAY_NUM, &TagDisplayNum, sizeof TagDisplayNum, 4))
	{
		s_nCurrentDisplay = m_nDisplay;
	}
#endif
}

unsigned CBcmFrameBuffer::GetNumDisplays (void)
{
#if RASPPI >= 4
	if (s_nDisplays > 0)
	{
		return s_nDisplays;
	}

	CBcmPropertyTags Tags;
	TPropertyTagSimple TagGetNumDisplays;
	if (Tags.GetTag (PROPTAG_GET_NUM_DISPLAYS, &TagGetNumDisplays, sizeof TagGetNumDisplays))
	{
		s_nDisplays = TagGetNumDisplays.nValue;
	}
	else
	{
		s_nDisplays = 1;
	}

	return s_nDisplays;
#else
	return 1;
#endif
}
