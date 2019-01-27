package com.wzjwhut.util.text;

import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;

import java.nio.ByteBuffer;

public class HexUtils {
    private final static Logger logger = LogManager.getLogger(HexUtils.class);

	private final static char[] chars = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};

	public static String hexString(byte[] bin) {
		if(bin == null){
			return "";
		}
		StringBuilder stringBuf = new StringBuilder(bin.length<<1);
		for (int i = 0; i < bin.length; i++) {
			stringBuf.append(chars[(bin[i]>>4)&0x0f]);
			stringBuf.append(chars[bin[i]&0x0f]);
		}
		return stringBuf.toString();
	}

	public static String hexString(byte[] bin, int offset, int len) {
		if(bin == null){
			return "";
		}
		StringBuilder stringBuf = new StringBuilder(bin.length<<1);
		for (int i = offset; i < offset + len; i++) {
			stringBuf.append(chars[(bin[i]>>4)&0x0f]);
			stringBuf.append(chars[bin[i]&0x0f]);
		}
		return stringBuf.toString();
	}

	public static void hexString(StringBuilder builder, byte[] bin, int offset, int len){
        for (int i = offset; i < offset + len; i++) {
            builder.append(chars[(bin[i]>>4)&0x0f]);
            builder.append(chars[bin[i]&0x0f]);
        }
    }

    public static void hexByte(StringBuilder builder, byte b){
        builder.append(chars[(b>>4)&0x0f]);
        builder.append(chars[b&0x0f]);
    }

    public static void hexShort(StringBuilder builder, short value){
	    byte high = (byte) (value>>8);
	    byte low = (byte)value;
        builder.append(chars[(high>>4)&0x0f]);
        builder.append(chars[high&0x0f]);
        builder.append(chars[(low>>4)&0x0f]);
        builder.append(chars[low&0x0f]);
    }

    public static String dumpString(byte[] bin) {
        if(bin == null){
            return "";
        }
        StringBuilder stringBuf = new StringBuilder(bin.length<<1);
        for (int i = 0; i < bin.length; i++) {
            stringBuf.append(chars[(bin[i]>>4)&0x0f]);
            stringBuf.append(chars[bin[i]&0x0f]);
            stringBuf.append(' ');
        }
        return stringBuf.toString();
    }

    public static String dumpString(byte[] bin, int offset, int len) {
        if(bin == null){
            return "";
        }
        StringBuilder stringBuf = new StringBuilder(bin.length<<1);
        for (int i = offset; i < offset + len; i++) {
            stringBuf.append(chars[(bin[i]>>4)&0x0f]);
            stringBuf.append(chars[bin[i]&0x0f]);
            stringBuf.append(' ');
        }
        return stringBuf.toString();
    }

    public static String dumpString(ByteBuffer buffer, int len){
        len = Math.min(len, buffer.remaining());
        int pos = buffer.position();
        byte[] content = new byte[len];
        buffer.get(content);
        buffer.position(pos);
        return dumpString(content);
    }

}
