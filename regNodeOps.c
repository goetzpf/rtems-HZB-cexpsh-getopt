/* $Id$ */

#define ECDR814_PRIVATE_IF
#include "regNodeOps.h"
#include "bitMenu.h"
#include "ecdrRegdefs.h"

#include "ecFastKeys.h"
#include "menuDefs.h"

#include <assert.h>
#include<stdio.h>

static EcErrStat
get(EcNode n, IOPtr b, Val_t *pv)
{
EcErrStat rval;
register Val_t val;

	if ( (n->u.r.flags & EcFlgAD6620RStatic) && ! ad6620IsReset(b)) {
		/* some AD6620 registers may only be read while it is in reset state */
		return EcErrAD6620NotReset;
	}
	rval=ecGetRawValue(n,b,pv);
	val = (*pv & ECREGMASK(n))>>ECREGPOS(n);
	if (!rval && EcFlgMenuMask & n->u.r.flags) {
		EcMenu		m=ecMenu(n->u.r.flags);
		EcMenuItem	mi;
		int		i;
		for (i=m->nels-1, mi=m->items+i; i>=0; i--,mi--) {
			if (mi->bitval == val)
				break;
		}
		if (i<0) return EcErrOutOfRange;
		val = i;
	}
	/* we must do range checking before adjusting the value
	 * (Val_t is unsigned, hence se must do the comparisons
	 * on the unsigned values)
	 */
	if (n->u.r.min != n->u.r.max &&
		( val < n->u.r.min + n->u.r.adj || val > n->u.r.max + n->u.r.adj ) )
		ecWarning( EcErrOutOfRange, " _read_ value of of range ???");
	val-=n->u.r.adj;
	*pv=val;

	return rval;
}

static EcErrStat
put(EcNode n, IOPtr b, Val_t val)
{

	if (n->u.r.flags & EcFlgReadOnly)
		return EcErrReadOnly;

	if ((n->u.r.flags & EcFlgAD6620RWStatic) && ! ad6620IsReset(b)) {
		return EcErrAD6620NotReset;
	}

	if (n->u.r.flags & EcFlgMenuMask) {
		EcMenu		m=ecMenu(n->u.r.flags);
		if (val < 0 || val>m->nels)
			return EcErrOutOfRange;
		val = m->items[val].bitval;
	}

	val += n->u.r.adj;

	/* see comment in 'get' about why we do comparison after adjustment */
	if (n->u.r.min != n->u.r.max &&
		( val < n->u.r.min + n->u.r.adj  || val > n->u.r.max + n->u.r.adj ) )
		return EcErrOutOfRange;


	{ /* merge into raw value */
	register EcErrStat	rval;
	register Val_t		m = ECREGMASK(n);
	Val_t 			rawval; 
		if (EcErrOK != (rval = ecGetRawValue(n,b,&rawval)))
			return rval;

		rawval = (rawval & ~m) | ((val<<ECREGPOS(n))&m);

		return ecPutRawValue(n,b,rawval);
	}
}

static EcErrStat
getRaw(EcNode n, IOPtr b, Val_t *rp)
{
volatile Val_t *vp = (Val_t *)b;
	*rp = RDBE(vp);
	return EcErrOK;
}


static EcErrStat
putRaw(EcNode n, IOPtr b, Val_t val)
{
volatile Val_t *vp = (Val_t *)b;
	WRBE(val,vp);
	return EcErrOK;
}

static EcErrStat
rdbckModePutRaw(EcNode n, IOPtr b, Val_t val)
{
/* TODO when writing the readback mode, we also want to check the fifo settings
 * and we have to switch unused channels off; actually, we do that
 * at a higher level...
 */
#if 0
EcErrStat	e;
unsigned long	fifoOff;
int		tmp,nk,i;
EcFKey		keys[10];
Val_t		v;
		/* calculate the default fifo offset */
		tmp = val-7;
		fifoOff = 16 >> ( tmp < 0 ? 0 : tmp );

		/* set the fifo offsets */
		/* first, we build keys */
		nk=0;
		keys[nk++]= BUILD_FKEY2(FK_board_01, FK_channelPair_fifoOffset);
		keys[nk++]= BUILD_FKEY2(FK_board_23, FK_channelPair_fifoOffset);
		keys[nk++]= BUILD_FKEY2(FK_board_45, FK_channelPair_fifoOffset);
		keys[nk++]= BUILD_FKEY2(FK_board_67, FK_channelPair_fifoOffset);
		/* set all the offsets */
		for (i=0; i<nk; i++) {
			if (e=ecLkupNPut(n, keys[i], b, val))
				return e;
		}
		/* turn off syncing of unused channels */
		/* build keys for all channels */
		nk=0;
		keys[nk++]= BUILD_FKEY3(FK_board_01,
					FK_channelPair_C0,
					FK_channel_trigMode);
		keys[nk++]= BUILD_FKEY3(FK_board_01,
					FK_channelPair_C1,
					FK_channel_trigMode);
		keys[nk++]= BUILD_FKEY3(FK_board_23,
					FK_channelPair_C0,
					FK_channel_trigMode);
		keys[nk++]= BUILD_FKEY3(FK_board_23,
					FK_channelPair_C1,
					FK_channel_trigMode);
		keys[nk++]= BUILD_FKEY3(FK_board_45,
					FK_channelPair_C0,
					FK_channel_trigMode);
		keys[nk++]= BUILD_FKEY3(FK_board_45,
					FK_channelPair_C1,
					FK_channel_trigMode);
		keys[nk++]= BUILD_FKEY3(FK_board_67,
					FK_channelPair_C0,
					FK_channel_trigMode);
		keys[nk++]= BUILD_FKEY3(FK_board_67,
					FK_channelPair_C1,
					FK_channel_trigMode);

		/* try to get current mode */
		for ( v = EC_MENU_trigMode_channelDisabled, i=0; 
		      EC_MENU_trigMode_channelDisabled==v && i < nk;
		      i++)
			if (e = ecLkupNGet(n, keys[i], b, &v))
				return e;
		if (EC_MENU_trigMode_channelDisabled == v)
			v = EC_MENU_trigMode_triggered; /* chose default */

		if (EC_MENU_rdbackMode_AllChannels != val) {
			/* turn off all channels */
			for (i=0; i< nk; i++) {
				if (e=ecLkupNPut(n,
						keys[i],
						b,
						EC_MENU_trigMode_channelDisabled))
					return e;
			}
			/* now selectively turn on channels */
			nk = 0;
			if ( val <= EC_MENU_rdbackMode_Channel_7  ) {
				/* one channel */
				keys[nk++] = keys[val];
			} else {
				/* at least two channels, 0 + 2 */
				nk=1;
				keys[nk++]=keys[2];
				if (EC_MENU_rdbackMode_Ch_0246) {
					/* four channels: 0,2,4,6 */
					keys[nk++] = keys[4];
					keys[nk++] = keys[6];
				} /* else must be two channels */
			}
		} /* else all channels are enabled */

		/* finally, enable the channels */
		for (i=0; i< nk; i++) {
			if (e=ecLkupNPut(n, keys[i], b, v))
				return e;
		}
#endif
		
		/* eventually write the board configuration register */
		return putRaw(n,b,val);
}

#if 0 /* DOESNT WORK (no access to the 'parent' node and base address) */
/* propagate the clock multiplier value to the "clockSame" bit */

static EcErrStat
clkMultPutRaw(EcNode n, IOPtr b, Val_t rawval)
{
EcErrStat	e;
int		hiPair,i;
Val_t		v;
EcFKey		k[2];

		/* is this request for channel 0..3 or for 4..7 ? */
		hiPair = n->u.r.pos1 > 17;

		/* reconstruct menu index and calculate "clockSame" */
		v = (EC_MENU_clockMult_Rx__ADC_Clk == ((rawval >> n->u.r.pos1) & 0x3));
		if (hiPair) {
			k[0]= BUILD_FKEY2(FK_board_45,FK_channelPair_clockSame);
			k[1]= BUILD_FKEY2(FK_board_67,FK_channelPair_clockSame);
		} else {
			k[0]= BUILD_FKEY2(FK_board_01,FK_channelPair_clockSame);
			k[1]= BUILD_FKEY2(FK_board_23,FK_channelPair_clockSame);
		}
		for (i=0; i<EcdrNumberOf(k); i++)
			if (e = ecLkupNPut(n, k[i], b, v))
				return e;

		/* eventually, write the board CSR */
		return putRaw(n,b,rawval);
}
#endif

#define FIFO_OFFREG	0x40
#define FIFOCTL_USE_OFF	(1<<5)
#define FIFOCTL_WRI_OFF (1<<4)
#define FIFOCTL_OFF_02  (3<<2)
#define FIFOCTL_OFF_04	(2<<2)
#define FIFOCTL_OFF_08  (1<<2)
#define FIFOCTL_OFF_16	(0<<2)
#define FIFOCTL_OFF_MSK	(3<<2)

static EcErrStat
fifoGetRaw(EcNode n, IOPtr b, Val_t *rp)
{
volatile Val_t *vp = (Val_t *)b;

Val_t	fifoctl = RDBE(vp);
	if (FIFOCTL_USE_OFF & fifoctl) {
		*rp = RDBE(((unsigned long)b)+FIFO_OFFREG);
	} else {
		switch (fifoctl & FIFOCTL_OFF_MSK) {
			case FIFOCTL_OFF_02: *rp = 2; break;
			case FIFOCTL_OFF_04: *rp = 4; break;
			case FIFOCTL_OFF_08: *rp = 8; break;
			case FIFOCTL_OFF_16: *rp = 16; break;
			default: *rp=0xdeadbeef; break; /* should never happen */
		}
	}
	return EcErrOK;
}


static EcErrStat
fifoPutRaw(EcNode n, IOPtr b, Val_t val)
{
volatile Val_t *vp = (Val_t *)b;

Val_t	fifoctl = RDBE(vp);
	fifoctl &= ~(FIFOCTL_USE_OFF | FIFOCTL_OFF_MSK);
		switch (val) {
			case 2:  fifoctl |= FIFOCTL_OFF_02; break;
			case 4:  fifoctl |= FIFOCTL_OFF_04; break;
			case 8:  fifoctl |= FIFOCTL_OFF_08; break;
			case 16: fifoctl |= FIFOCTL_OFF_16; break;
			default: fifoctl |= FIFOCTL_USE_OFF;
				 WRBE(val, ((unsigned long)b)+FIFO_OFFREG);
				 break;
		}
	fifoctl |= FIFOCTL_WRI_OFF;
	WRBE(fifoctl,vp);
	return EcErrOK;
}

#define BURST_COUNT_LSBREG 0x8
#define BURST_COUNT_MSB	1

static EcErrStat
brstCntGetRaw(EcNode n, IOPtr b, Val_t *rp)
{
volatile Val_t *vp = (Val_t*)(((unsigned long)b)+BURST_COUNT_LSBREG);

	/* get LSB */
	*rp = RDBE(vp);

	/* get MSB */
	vp = (Val_t*)b;
	if (RDBE(vp) & BURST_COUNT_MSB)
		*rp|=(1<<16);
	else 
		*rp&=~(1<<16);
	return EcErrOK;
}


static EcErrStat
brstCntPutRaw(EcNode n, IOPtr b, Val_t val)
{
volatile Val_t *vp = (Val_t*)(((unsigned long)b)+BURST_COUNT_LSBREG);
Val_t		msb;
	/* write LSB */
	WRBE((val&0xffff), vp);
	/* write MSB */
	vp = (Val_t *)b;
	msb = RDBE(vp);
	if (val & (1<<16))
		msb |= BURST_COUNT_MSB;
	else 
		msb &= ~BURST_COUNT_MSB;
	WRBE(msb,vp);
	return EcErrOK;
}


static EcErrStat
adGetRaw(EcNode n, IOPtr b, Val_t *rp)
{
volatile Val_t *vp = (Val_t *)b;

#if 0 /* doing this in 'get' now */
	/* filter coefficients must not be dynamically read
	 * (AD6620 datasheet rev.1)
	 *
	 * Note that we should prohibit access to the NCO
	 * frequency as well if in SYNC_MASTER mode, as this
	 * would cause a sync pulse. However, we assume
	 * the AD6620s on an ECDR board to always be sync
	 * slaves.
	 */
	if (n->u.r.flags & EcFlgAD6620RStatic) {
		/* calculate address of MCR */
		register volatile Val_t *mcrp = (Val_t*)(AD6620BASE(b) + OFFS_AD6620_MCR);
		register long	isreset = (RDBE(mcrp) & BITS_AD6620_MCR_RESET);
			EIEIO; /* make sure there is no out of order hardware access beyond
				* this point
				*/
			if (!isreset) return EcErrAD6620NotReset;
	}
#endif
	/* be careful that the compiler doesn't generate an unaligned access */
	*rp = (RDBE(vp) & 0xffff) | (((RDBE(vp+1)) & 0xffff)<<16) ;
	return EcErrOK;
}


static EcErrStat
adPutMCR(EcNode n, IOPtr b, Val_t val)
{

	if (n->u.r.flags & EcFlgReadOnly)
		return EcErrReadOnly;

	val += n->u.r.adj;

	/* see comment in 'get' about why we do comparison after adjustment */
	if (n->u.r.min != n->u.r.max &&
		( val < n->u.r.min + n->u.r.adj  || val > n->u.r.max + n->u.r.adj ) )
		return EcErrOutOfRange;

	{ /* merge into raw value */
	register EcErrStat	rval;
	register Val_t		m = ECREGMASK(n);
	Val_t 			orawval, rawval; 
		if (EcErrOK != (rval = ecGetRawValue(n,b,&orawval)))
			return rval;

		/* merge old raw value and val into raw value */
		rawval = (orawval & ~m) | ((val<<ECREGPOS(n))&m);

		/* now let's see what they intend to do */
		orawval = rawval ^ orawval; /* look at what bits changed */
		if ( (orawval & ~ BITS_AD6620_MCR_RESET) &&		/* bits other than reset have changed */
		     ((orawval | ~rawval) & BITS_AD6620_MCR_RESET) ) {	/* reset has also changed or is low */
				/* reject attempt to change */
				return EcErrAD6620NotReset;
		} else {
			/* only reset has changed (or noting at all) */
			if ( ! (rawval&BITS_AD6620_MCR_RESET) ) {
				/* taking it out of reset, do consistency check */
				rval=ad6620ConsistencyCheck(n, b);
				EIEIO;
				if (rval) return rval; /* reject inconsistent configuration */
			}
		}

		return ecPutRawValue(n,b,rawval);
	}
}

static EcErrStat
adPutRaw(EcNode n, IOPtr b, Val_t val)
{
volatile Val_t	*vp = (Val_t *)b;

#if 0        /* this is now done by the higher level routines */
	/* allow write attempt to "static" 
	 * registers only if the receiver is in RESET
	 * state. At this low level, we leave the abstraction
	 * behind and dive directly into the guts...
	 */
	if ( (n->u.r.flags & EcFlgAD6620RWStatic) ) {

		/* must inspect current state of RESET bit */

		register unsigned long	mcr = RDBE((AD6620BASE(b) + OFFS_AD6620_MCR));

		/* it's OK to do the following:
		 *  if in reset state
		 *  - change any other register than MCR
		 *  - change either the reset bit or any other bit in MCR
		 *    but not both.
		 *  if we're out of reset
		 *  - the only allowed operation
		 *    is just setting the reset bit
		 */
		if ( (BITS_AD6620_MCR_RESET & mcr) ) {
			if ( OFFS_AD6620_MCR == n->offset ) {
				/* trying to modify mcr; let's see which bits change: */
				mcr = (val^mcr) & BITS_AD6620_MCR_MASK;
				if ( mcr & BITS_AD6620_MCR_RESET) {
					/* reset bit switched off */
					if ( mcr & ~BITS_AD6620_MCR_RESET) {
						/* another bit changed as well */
						return EcErrAD6620NotReset;
					}
					/* TODO do consistency check on all settings */
				}
				/* other MCR bits changed while reset held */
			}
		} else {
			/* only flipping reset is allowed */
			if ( OFFS_AD6620_MCR != n->offset  ||
			    ((val ^ mcr) & BITS_AD6620_MCR_MASK)
			     != BITS_AD6620_MCR_RESET )
				return EcErrAD6620NotReset;
		}
		EIEIO; /* make sure there is no out of order hardware access beyond
			* this point
			*/
	}
#endif

	EIEIO; /* make sure read of old value completes */
	WRBE(val & 0xffff, vp);
	EIEIO; /* most probably not necessary in guarded memory */
	WRBE((val>>16)&0xffff, vp+1);
	EIEIO; /* just in case they read it back again: enforce
	        * completion of write before load
	        */
	return EcErrOK;
}

static EcErrStat
getCoeffs(EcNode n, IOPtr b, Val_t *v)
{
volatile unsigned long *s = (volatile unsigned long *) b;
unsigned long *d = (unsigned long *) v;
int i;
EcErrStat e;
for (i=n->u.r.pos1; i < n->u.r.pos2; i++,d++,s+=2)
	if (e=adGetRaw(n,(IOPtr)s,(Val_t*)d))
		return e;
return EcErrOK;
}

static EcErrStat
putCoeffs(EcNode n, IOPtr b, Val_t v)
{
unsigned long *s = (unsigned long *) v;
volatile unsigned long *d = (volatile unsigned long *) b;
int i;
EcErrStat e;
	if ( ! ad6620IsReset(b)) {
		/* some coefficients may only be read while it is in reset state */
		return EcErrAD6620NotReset;
	}
	for (i=n->u.r.pos1; i < n->u.r.pos2; i++,s++,d+=2)
		if (e=adPutRaw(n,(IOPtr)d,*(Val_t*)s))
			return e;
return EcErrOK;
}

/* consistency check for decimation factors and number of taps */

/*
{
nch = 2 : diversity channel real input, 1 otherwise.

ECHOTEK: I believe it's always in single channel real mode

cic5:  input rate < fclk / 2 / nch
;

i.e. CIC2 decimation of 1 requires RX clock >= 2 * acquisition clock;
ECHOTEK: I don't know how the receiver_clock_same (channel pair
setting) relates to the rx_clock_multiplier0..3 / 4..7 (base
board setting).

NTAPS requirement:

ntaps < min( 256, fclk * Mrcf / fclk5) / nch
 = min(256, fclk * Mrcf * Mcic5 * Mcic2 / fclck) / nch

ECHOTEK:
	total decimation register must be set to
        Mcic2*Mcic5*Mrcf/2 - 1
	(what to do if it's not even?)

furthermore:
	1stTap+ntaps <= 256

fclk5 = fclk2 / Mcic5

ECHOTEK: if in dual receiver mode, the number of taps must be even.
         However, I believe this restriction is only necessary
         when "interleaving" the dual RX output samples.

         When tuning both AD6620 to different frequencies, they
         recommend the output rates being integer multiples.

}
*/

EcNodeOpsRec	ecDefaultNodeOps = {
	0, /* super */
	0,
	0,
	0,
	0,
};


EcNodeOpsRec ecdr814RegNodeOps = {
	&ecDefaultNodeOps,
	0,
	get,
	getRaw,
	put,
	putRaw
};

EcNodeOpsRec ecdr814AD6620RegNodeOps = {
	&ecdr814RegNodeOps,
	0,
	0,
	adGetRaw,
	0,
	adPutRaw
};

EcNodeOpsRec ecdr814AD6620RCFNodeOps = {
	&ecdr814AD6620RegNodeOps,
	0,
	getCoeffs,
	0,
	putCoeffs,
	0
};

EcNodeOpsRec ecdr814AD6620MCRNodeOps = {
	&ecdr814AD6620RegNodeOps,
	0,
	0,
	0,
	adPutMCR,
	0
};

EcNodeOpsRec ecdr814BrstCntRegNodeOps = {
	&ecdr814RegNodeOps,
	0,
	0,
	brstCntGetRaw,
	0,
	brstCntPutRaw
};

EcNodeOpsRec ecdr814FifoRegNodeOps = {
	&ecdr814RegNodeOps,
	0,
	0,
	fifoGetRaw,
	0,
	fifoPutRaw
};

EcNodeOpsRec ecdr814RdBckRegNodeOps = {
	&ecdr814RegNodeOps,
	0,
	0,
	0,
	0,
	rdbckModePutRaw
};


static EcNodeOps nodeOps[10]={&ecDefaultNodeOps,};

/* public routines to access leaf nodes */

EcErrStat
ecGetValue(EcNode n, IOPtr b, Val_t *vp)
{
EcNodeOps ops;
	assert(n->t < EcdrNumberOf(nodeOps) && (ops=nodeOps[n->t]));
	assert(ops->get);
	return ops->get(n,b,vp);
}

EcErrStat
ecGetRawValue(EcNode n, IOPtr b, Val_t *vp)
{
EcNodeOps ops;
	assert(n->t < EcdrNumberOf(nodeOps) && (ops=nodeOps[n->t]));
	assert(ops->getRaw);
	return ops->getRaw(n,b,vp);
}

EcErrStat
ecPutValue(EcNode n, IOPtr b, Val_t val)
{
EcNodeOps ops;
	assert(n->t < EcdrNumberOf(nodeOps) && (ops=nodeOps[n->t]));
	assert(ops->put);
	return ops->put(n,b,val);
}

EcErrStat
ecPutRawValue(EcNode n, IOPtr b, Val_t val)
{
EcNodeOps ops;
	assert(n->t < EcdrNumberOf(nodeOps) && (ops=nodeOps[n->t]));
	assert(ops->putRaw);
	return ops->putRaw(n,b,val);
}

EcErrStat
ecLkupNGet(EcNode n, EcFKey k, IOPtr base, Val_t *pv)
{
IOPtr addr=base;
	if (!(n=lookupEcNodeFast(n, k, &addr, 0))) {
		return EcErrNodeNotFound;
	}
	return ecGetValue(n, addr, pv);
}

EcErrStat
ecLkupNPut(EcNode n, EcFKey k, IOPtr base, Val_t pv)
{
IOPtr addr=base;
if (!(n=lookupEcNodeFast(n, k, &addr, 0))) {
	return EcErrNodeNotFound;
}
return ecPutValue(n, addr, pv);
}

static void
recursive_ini(EcNodeOps ops)
{
EcNodeOps super=ops->super;
	if (super) {
		if (!super->initialized)
			recursive_ini(super);
		if (!ops->get) ops->get = super->get;
		if (!ops->getRaw) ops->getRaw = super->getRaw;
		if (!ops->put) ops->put = super->put;
		if (!ops->putRaw) ops->putRaw = super->putRaw;
	}
	ops->initialized=1;
}

void
addNodeOps(EcNodeOps ops, EcNodeType t)
{
	assert( t < EcdrNumberOf(nodeOps) && !nodeOps[t]);
	recursive_ini(ops);
	nodeOps[t] = ops;
}

void
initRegNodeOps(void)
{
	addNodeOps(&ecdr814RegNodeOps, EcReg);
	addNodeOps(&ecdr814AD6620RegNodeOps, EcAD6620Reg);
	addNodeOps(&ecdr814AD6620MCRNodeOps, EcAD6620MCR);
	addNodeOps(&ecdr814BrstCntRegNodeOps, EcBrstCntReg);
	addNodeOps(&ecdr814FifoRegNodeOps, EcFifoReg);
	addNodeOps(&ecdr814RdBckRegNodeOps, EcRdBckReg);
	addNodeOps(&ecdr814AD6620RCFNodeOps, EcAD6620RCF);
}
