package com.wzjwhut.protoc;

import java.io.BufferedWriter;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.OutputStreamWriter;
import java.util.HashMap;
import java.util.LinkedList;
import java.util.List;
import java.util.Set;

import org.apache.commons.io.FileUtils;
import org.apache.commons.io.FilenameUtils;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;

import com.wzjwhut.util.text.HexUtils;
import com.wzjwhut.util.text.StringFormatter;
import com.google.protobuf.DescriptorProtos.DescriptorProto;
import com.google.protobuf.DescriptorProtos.EnumDescriptorProto;
import com.google.protobuf.DescriptorProtos.EnumValueDescriptorProto;
import com.google.protobuf.DescriptorProtos.FieldDescriptorProto;
import com.google.protobuf.DescriptorProtos.FieldDescriptorProto.Type;
import com.google.protobuf.DescriptorProtos.FileDescriptorProto;
import com.google.protobuf.DescriptorProtos.FileDescriptorSet;
import com.google.protobuf.Descriptors.FieldDescriptor;
import com.google.protobuf.WireFormat;

public class Main {
	private final static Logger logger = LogManager.getLogger(Main.class);

	public static void main(String[] args) throws Exception {
		String protoPath = "";
		if(args.length>0) {
			protoPath = new File(args[0]).getAbsolutePath();
		}else{
            System.out.println("usage\r\n" +
                    "java -jar protoc.jar your.proto\r\n");
            protoPath = "./sample/MyProto.proto";
        }
		
		String workdir = System.getProperty("user.dir");
		logger.info("work dir: {}", workdir);
		String tempDir = workdir.charAt(0) + ":" + File.separator + "temp" + File.separator;
		new File(tempDir).mkdirs();
		
		File file = new File(protoPath);
		String dir = file.getParent();
		String fileName = file.getName();
		String fileNameNoExt = FilenameUtils.getBaseName(fileName);
		String protoC = dir + File.separator + "protoc.exe";
		if(!new File(protoC).exists()){
		    System.out.println("not found protoc.exe !!!");
		    return;
        }
		String protoCtemp = tempDir + File.separator + "protoc.exe";
		String descPathDest = dir + File.separator + file.getName() + ".desc";
		String tempProto = tempDir + fileName;
		String tempDesc = tempDir + fileName + ".desc";
		String tempC = tempDir + fileNameNoExt + ".c";
		String tempH = tempDir + fileNameNoExt + ".h";
		String destC = dir + File.separator + fileNameNoExt + ".c";
		String destH = dir + File.separator + fileNameNoExt + ".h";
		FileUtils.copyFile(new File(protoPath), new File(tempProto));
		FileUtils.copyFile(new File(protoC), new File(protoCtemp));
		Runtime run = Runtime.getRuntime();
		String cmd = "cmd /c " + protoCtemp + " -I=" + tempDir + " --descriptor_set_out=" + tempDesc + " " + tempProto;
		logger.info("cmd: {}", cmd);
		Process p = run.exec(cmd);
		// 如果不正常终止, 则生成desc文件失败
		if (p.waitFor() != 0) {
			if (p.exitValue() == 1) {// p.exitValue()==0表示正常结束，1：非正常结束
				logger.error("命令执行失败!");
				System.exit(1);
			}
		}
		FileUtils.copyFile(new File(tempDesc), new File(descPathDest));
		logger.info("protoc success");
		BufferedWriter C = new BufferedWriter(new OutputStreamWriter(new FileOutputStream(new File(tempC))));
		BufferedWriter H = new BufferedWriter(new OutputStreamWriter(new FileOutputStream(new File(tempH))));

		FileInputStream fin = new FileInputStream(descPathDest);
		FileDescriptorSet descriptorSet = FileDescriptorSet.parseFrom(fin);
		FileDescriptorSet.parseFrom(fin);

		StringBuilder h_define = new StringBuilder();
		StringBuilder h_function = new StringBuilder();
		StringBuilder c_function = new StringBuilder();
		
		
		HashMap<String, DescriptorProto> sortMap = new HashMap<>();
		LinkedList<String> sortList = new LinkedList<>();

		/** 收集信息, 并排序 */
		for (FileDescriptorProto fdp : descriptorSet.getFileList()) {
			Set<FieldDescriptor> fields = fdp.getAllFields().keySet();
			for (FieldDescriptor field : fields) {
				Object value = fdp.getField(field);
				logger.info("values cls: {}, name: {}", value.getClass(), field.getFullName());
				if (value instanceof List) {
					List<DescriptorProto> protos = (List<DescriptorProto>) value;
					for (DescriptorProto proto : protos) {
						logger.info("=============== {} ================", proto.getName());
						sortMap.put(proto.getName(), proto);
						int protoIndex = sortList.indexOf(proto.getName());
						if(protoIndex == -1) {
							sortList.addLast(proto.getName());
							protoIndex = sortList.size()-1;
						}
						
						List<FieldDescriptorProto> fieldList = proto.getFieldList();
						for (FieldDescriptorProto pro : fieldList) {
							Type type = pro.getType();
							if(type == Type.TYPE_MESSAGE) {
								/** 有依赖, 被依赖方放在前面 */
								String fieldTypeName = typename(pro.getTypeName());
								int pos = sortList.indexOf(fieldTypeName);
								if(pos == -1 ) {
									sortList.add(sortList.indexOf(proto.getName()), fieldTypeName);
								}else if(pos < protoIndex) {
									//顺序是对的.
								}else {
									//顺序不对, 再排
									sortList.remove(fieldTypeName);
									sortList.add(sortList.indexOf(proto.getName()), fieldTypeName);
								}
							} 
						}
					}
				}
			}
		}	
		LinkedList<DescriptorProto> protos = new LinkedList<>();
		for(String type: sortList) {
			logger.info("{}", type);
			DescriptorProto proto = sortMap.get(type);
			if(proto==null) {
				throw new RuntimeException("");
			}
			protos.addLast(sortMap.get(type));
		}
		logger.info("protos: {}", protos.size());
		
		
		/** 处理结构体声明 */
		for (DescriptorProto proto : protos) {
			logger.info("=============== {} ================", proto.getName());
			List<EnumDescriptorProto> enums = proto.getEnumTypeList();
			if (enums != null) {
				for (EnumDescriptorProto enumProto : enums) {
					logger.info("enum: {}", enumProto.getName());
					h_define.append(StringFormatter.format("typedef enum {} {\r\n", enumProto.getName()));

					List<EnumValueDescriptorProto> values = enumProto.getValueList();
					for (EnumValueDescriptorProto enumValue : values) {
						logger.info("neum value: {}, {}", enumValue.getName(), enumValue.getNumber());
						h_define.append(StringFormatter.format("    {}_{} = {},\r\n", enumProto.getName(), enumValue.getName(),
								enumValue.getNumber()));
					}
					h_define.append(StringFormatter.format("} {};\r\n", enumProto.getName()));

				}
			}

			h_define.append(StringFormatter.format(
					        "typedef struct {} {\r\n" + 
							"    pb_message _imessage;\r\n"+
					        "    uint32_t _has_bits_[1];\r\n" + 
							"    size_t _cached_size_;\r\n"+ 
					        "    PBOOL _inited;\r\n"
							,
					proto.getName()));
			List<FieldDescriptorProto> fieldList = proto.getFieldList();
			for (FieldDescriptorProto pro : fieldList) {
				Type type = pro.getType();
				if (type == Type.TYPE_BOOL) {
					h_define.append("    PBOOL " + pro.getName() + ";\r\n");
				} else if (type == Type.TYPE_BYTES) {
					h_define.append("    pb_string " + pro.getName() + ";\r\n");
				} else if (type == Type.TYPE_STRING) {
					h_define.append("    pb_string " + pro.getName() + ";\r\n");
				} else if (type == Type.TYPE_ENUM) {
					h_define.append("    " + typename(pro.getTypeName()) + " " + pro.getName() + ";\r\n");
				} else if (type == Type.TYPE_UINT32) {
					h_define.append("    uint32_t " + pro.getName() + ";\r\n");
				} else if (type == Type.TYPE_UINT64) {
					h_define.append("    uint64_t " + pro.getName() + ";\r\n");
				} else if (type == Type.TYPE_INT64) {
					h_define.append("    int64_t " + pro.getName() + ";\r\n");
				} else if (type == Type.TYPE_INT32) {
					h_define.append("    int32_t " + pro.getName() + ";\r\n");
				} else if (type == Type.TYPE_MESSAGE) {
					h_define.append("    " + typename(pro.getTypeName()) + " " + pro.getName() + ";\r\n");
				} else {
					throw new RuntimeException("unsupport " + type);
				}
			}
			h_define.append(StringFormatter.format("} {};\r\n", proto.getName()));
		}
		logger.info("define: \r\n{}", h_define.toString());

		/** 处理函数体 */
		for (DescriptorProto proto : protos) {
			logger.info("=============== {} ================", proto.getName());

			h_function.append(StringFormatter.format("void {}_init({}* proto);\r\n",proto.getName(), proto.getName()));
			
			List<FieldDescriptorProto> fieldList = proto.getFieldList();
			long bits = 1L;
			int loopCount = 0;
			for (FieldDescriptorProto field : fieldList) {
				Type type = field.getType();
				h_function.append(StringFormatter.format(
						"inline static PBOOL {}_has_{}({}* proto) {\r\n" + 
						"    return (proto->_has_bits_[0] & 0x{}u) != 0;\r\n" + 
						"}\r\n",proto.getName(), field.getName(), proto.getName(), Long.toHexString(bits)));
				h_function.append(StringFormatter.format(
						"inline static void {}_set_has_{}({}* proto) {\r\n" + 
						"    proto->_has_bits_[0] |= 0x{}u;\r\n" + 
						"}\r\n",proto.getName(), field.getName(), proto.getName(), Long.toHexString(bits)));
				if (type == Type.TYPE_BOOL) {
					h_function.append(StringFormatter.format(
							"inline static void {}_set_{}({}* proto, const PBOOL value){\r\n" + 
							"    proto->_has_bits_[0] |= 0x{}u;\r\n" + 
							"    proto->{} = value;\r\n" + 
							"}\r\n"
							,proto.getName(), field.getName(), proto.getName(), Long.toHexString(bits), field.getName()));								
				} 
				else if (type == Type.TYPE_BYTES) {
					h_function.append(StringFormatter.format(
							"inline static void {}_set_{}({}* proto, const pb_string* value){\r\n" + 
							"    proto->_has_bits_[0] |= 0x{}u;\r\n" + 
							"    proto->{} = *value;\r\n" + 
							"}\r\n"
							,proto.getName(), field.getName(), proto.getName(), Long.toHexString(bits), field.getName()));			
				} 
				else if (type == Type.TYPE_STRING) {
					h_function.append(StringFormatter.format(
							"inline static void {}_set_{}({}* proto, const pb_string* value){\r\n" + 
							"    proto->_has_bits_[0] |= 0x{}u;\r\n" + 
							"    proto->{} = *value;\r\n" + 
							"}\r\n"
							,proto.getName(), field.getName(), proto.getName(), Long.toHexString(bits), field.getName()));	
				} else if (type == Type.TYPE_ENUM) {
					h_function.append(StringFormatter.format(
							"inline static void {}_set_{}({}* proto, const {} value){\r\n" + 
							"    proto->_has_bits_[0] |= 0x{}u;\r\n" + 
							"    proto->{} = value;\r\n" + 
							"}\r\n"
							,proto.getName(), field.getName(), proto.getName(), typename(field.getTypeName()),Long.toHexString(bits), field.getName()));		
				} else if (type == Type.TYPE_UINT32) {
					h_function.append(StringFormatter.format(
							"inline static void {}_set_{}({}* proto, const uint32_t value){\r\n" + 
							"    proto->_has_bits_[0] |= 0x{}u;\r\n" + 
							"    proto->{} = value;\r\n" + 
							"}\r\n"
							,proto.getName(), field.getName(), proto.getName(), Long.toHexString(bits), field.getName()));		
				} else if (type == Type.TYPE_UINT64) {
					h_function.append(StringFormatter.format(
							"inline static void {}_set_{}({}* proto, const uint64_t value){\r\n" + 
							"    proto->_has_bits_[0] |= 0x{}u;\r\n" + 
							"    proto->{} = value;\r\n" + 
							"}\r\n"
							,proto.getName(), field.getName(), proto.getName(), Long.toHexString(bits), field.getName()));		
				} else if (type == Type.TYPE_INT64) {
					h_function.append(StringFormatter.format(
							"inline static void {}_set_{}({}* proto, const int64_t value){\r\n" + 
							"    proto->_has_bits_[0] |= 0x{}u;\r\n" + 
							"    proto->{} = value;\r\n" + 
							"}\r\n"
							,proto.getName(), field.getName(), proto.getName(), Long.toHexString(bits), field.getName()));		
				} else if (type == Type.TYPE_INT32) {
					h_function.append(StringFormatter.format(
							"inline static void {}_set_{}({}* proto, const int32_t value){\r\n" + 
							"    proto->_has_bits_[0] |= 0x{}u;\r\n" + 
							"    proto->{} = value;\r\n" + 
							"}\r\n"
							,proto.getName(), field.getName(), proto.getName(), Long.toHexString(bits), field.getName()));		
				} else if (type == Type.TYPE_MESSAGE) {
					h_function.append(StringFormatter.format(
							"inline static void {}_set_{}({}* proto, const {}* value){\r\n" + 
							"    proto->_has_bits_[0] |= 0x{}u;\r\n" + 
							"    proto->{} = *value;\r\n" + 
							"}\r\n"
							,proto.getName(), field.getName(), proto.getName(), typename(field.getTypeName()),Long.toHexString(bits), field.getName()));	
				} else {
					throw new RuntimeException("unsupport " + type);
				}
				bits = bits<<1;
				loopCount++;
			}
			if(loopCount>32) {
				throw new RuntimeException("has tow many fields: " + proto.getName());
			}

			h_function.append(StringFormatter.format(
					"inline static uint8_t* {}_to_byte_array({}* proto, int* out_len){\r\n" + 
					"    return pb_message_to_byte_array(&proto->_imessage, out_len);\r\n" + 
					"}\r\n" + 
					"\r\n" + 
					"inline static void {}_from_byte_array({}* proto, uint8_t* data,\r\n" + 
					"                              int len){\r\n" + 
					"    pb_message_from_byte_array(&proto->_imessage, data, len);\r\n" + 
					"}\r\n" +
					"inline static size_t {}_bytes_size({}* proto){\r\n" + 
					"    return proto->_imessage.ByteSizeLong(&proto->_imessage);\r\n" + 
					"}\r\n" + 
					"\r\n" + 
					"inline static int {}_serialize_to({}* proto, uint8_t* dest, int dest_len){\r\n" + 
					"    return pb_message_serialize_to(&proto->_imessage, dest, dest_len);\r\n" + 
					"}\r\n" ,
					proto.getName(), proto.getName(), proto.getName(), proto.getName(), proto.getName(), proto.getName(), proto.getName(), proto.getName()));		
		}
		


		for (DescriptorProto proto : protos) {
			logger.info("=============== {} ================", proto.getName());

			c_function.append(StringFormatter.format(
					"inline static int {}_GetCachedSize(struct pb_message* message){\r\n" + 
					"    {}* thiz = ({}*)message;\r\n" + 
					"    return (int)thiz->_cached_size_;\r\n" + 
					"}\r\n",
					proto.getName(),proto.getName(), proto.getName()));
			
			c_function.append(StringFormatter.format(
					"inline static void {}_Serialize(struct pb_message* message,pb_outstream* s){\r\n" + 
					"    {}* thiz = ({}*)message;\r\n" ,
					proto.getName(),proto.getName(), proto.getName()));
	
			
			List<FieldDescriptorProto> fieldList = proto.getFieldList();
			
			for (FieldDescriptorProto field : fieldList) {
				Type type = field.getType();
				c_function.append(StringFormatter.format("    if({}_has_{}(thiz)){\r\n", proto.getName(), field.getName()));
				if (type == Type.TYPE_BOOL) {
					c_function.append(StringFormatter.format(
							"        WriteBool(s, {}, thiz->{});\r\n"
							,field.getNumber(), field.getName()));								
				} 
				else if (type == Type.TYPE_BYTES) {
					c_function.append(StringFormatter.format(
							"        WriteBytes(s, {}, &thiz->{});\r\n"
							,field.getNumber(), field.getName()));				
				} 
				else if (type == Type.TYPE_STRING) {
					c_function.append(StringFormatter.format(
							"        WriteString(s, {}, &thiz->{});\r\n"
							,field.getNumber(), field.getName()));		
				} else if (type == Type.TYPE_ENUM) {
					c_function.append(StringFormatter.format(
							"        WriteEnum(s, {}, (int32_t)thiz->{});\r\n"
							,field.getNumber(), field.getName()));		
				} else if (type == Type.TYPE_UINT32) {
					c_function.append(StringFormatter.format(
							"        WriteUint32(s, {}, thiz->{});\r\n"
							,field.getNumber(), field.getName()));		
				} else if (type == Type.TYPE_UINT64) {
					c_function.append(StringFormatter.format(
							"        WriteUint64(s, {}, thiz->{});\r\n"
							,field.getNumber(), field.getName()));		
				} else if (type == Type.TYPE_INT64) {
					c_function.append(StringFormatter.format(
							"        WriteInt64(s, {}, thiz->{});\r\n"
							,field.getNumber(), field.getName()));		
				} else if (type == Type.TYPE_INT32) {
					c_function.append(StringFormatter.format(
							"        WriteInt32(s, {}, thiz->{});\r\n"
							,field.getNumber(), field.getName()));		
				} else if (type == Type.TYPE_MESSAGE) {
					c_function.append(StringFormatter.format(
							"        WriteMessage(s, {},(pb_message*)&thiz->{});\r\n"
							,field.getNumber(), field.getName()));		
				} else {
					throw new RuntimeException("unsupport " + type);
				}
				c_function.append("    }\r\n");
			}	
			c_function.append("}\r\n");
			
			c_function.append(StringFormatter.format(
					"inline static size_t {}_ByteSizeLong(struct pb_message* message){\r\n" + 
					"    {}* thiz = ({}*)message;\r\n" +
					"    size_t total_size = 0;\r\n" +
					"    if(thiz->_cached_size_ != (size_t)(-1)){\r\n" + 
					"        return thiz->_cached_size_;\r\n" + 
					"    }\r\n"
					,
					proto.getName(),proto.getName(), proto.getName()));
	
		
			for (FieldDescriptorProto field : fieldList) {
				Type type = field.getType();
				int tagSize = 1;
				int fieldNumber = field.getNumber();
				if(fieldNumber<=15) {
					tagSize = 1;
				}else if(fieldNumber>=16 && fieldNumber<=0x7FF) {
					tagSize = 2;
				}else {
					throw new RuntimeException("unsupport filed number: " + fieldNumber + ", field: " + field.getName());
				}
				
				c_function.append(StringFormatter.format("    if({}_has_{}(thiz)){\r\n", proto.getName(), field.getName()));
				if (type == Type.TYPE_BOOL) {						
					c_function.append(StringFormatter.format(
							"        total_size += {} + BoolSize(thiz->{});\r\n"
							,tagSize, field.getName()));								
				} 
				else if (type == Type.TYPE_BYTES) {
					c_function.append(StringFormatter.format(
							"        total_size += {} + BytesSize(&thiz->{});\r\n"
							,tagSize, field.getName()));				
				} 
				else if (type == Type.TYPE_STRING) {
					c_function.append(StringFormatter.format(
							"        total_size += {} + StringSize(&thiz->{});\r\n"
							,tagSize, field.getName()));	
				} else if (type == Type.TYPE_ENUM) {
					c_function.append(StringFormatter.format(
							"        total_size += {} + EnumSize((int)thiz->{});\r\n"
							,tagSize, field.getName()));	
				} else if (type == Type.TYPE_UINT32) {
					c_function.append(StringFormatter.format(
							"        total_size += {} + Int32Size(thiz->{});\r\n"
							,tagSize, field.getName()));	
				} else if (type == Type.TYPE_UINT64) {
					c_function.append(StringFormatter.format(
							"        total_size += {} + UInt64Size(thiz->{});\r\n"
							,tagSize, field.getName()));
				} else if (type == Type.TYPE_INT64) {
					c_function.append(StringFormatter.format(
							"        total_size += {} + Int64Size(thiz->{});\r\n"
							,tagSize, field.getName()));
				} else if (type == Type.TYPE_INT32) {
					c_function.append(StringFormatter.format(
							"        total_size += {} + Int32Size(thiz->{});\r\n"
							,tagSize, field.getName()));
				} else if (type == Type.TYPE_MESSAGE) {
					c_function.append(StringFormatter.format(
							"        total_size += {} + MessageSize((pb_message*)&thiz->{});\r\n"
							,tagSize, field.getName()));
				} else {
					throw new RuntimeException("unsupport " + type);
				}
				c_function.append("    }\r\n");
			}
			c_function.append("    thiz->_cached_size_ = total_size;\r\n" + 
					"    return thiz->_cached_size_;\r\n"
					+ "}\r\n");
			
			
			c_function.append(StringFormatter.format(
					"inline static PBOOL {}_mergeFromInputStream(struct pb_message* message, pb_inputstream* s){\r\n" + 
				    " #define DO_(EXPRESSION) if (!(EXPRESSION)) goto failure\r\n" +
					"    {}* thiz = ({}*)message;\r\n" +
					"    uint32_t tag = -1;\r\n" + 
					"    for (;;) {\r\n" + 
					"        if(!ReadTag(s, &tag)){\r\n" + 
					"            goto handle_unusual;\r\n" + 
					"        }\r\n" + 
					"        switch(GetTagFieldNumber(tag)){\r\n"
					,
					proto.getName(),proto.getName(), proto.getName()));
	
		
			for (FieldDescriptorProto field : fieldList) {
				Type type = field.getType();
				int tagSize = 1;
				int fieldNumber = field.getNumber();
				if(fieldNumber<=15) {
					tagSize = 1;
				}else if(fieldNumber>=16 && fieldNumber<=0x7FF) {
					tagSize = 2;
				}else {
					throw new RuntimeException("unsupport filed number: " + fieldNumber + ", field: " + field.getName());
				}
				//logger.info("fieldNumber: {}, type: {}",fieldNumber, type.TYPE_DOUBLE_VALUE);
				int tag ;
	
				if (type == Type.TYPE_BOOL) {	
					tag = MakeTag(fieldNumber, WireFormat.WIRETYPE_VARINT);
					c_function.append(StringFormatter.format(
							"        case {}:{\r\n" + 
							"            if(tag == {}){\r\n" + 
							"                {}_set_has_{}(thiz);\r\n" + 
							"                DO_(ReadBool(s, &thiz->{}));\r\n" + 
							"            }else{\r\n" + 
							"                goto handle_unusual;\r\n" + 
							"            }\r\n" + 
							"            break;\r\n" + 
							"        }\r\n"
							, fieldNumber, tag, proto.getName(), field.getName(), field.getName()));
				} 
				else if (type == Type.TYPE_BYTES) {
					tag = MakeTag(fieldNumber, WireFormat.WIRETYPE_LENGTH_DELIMITED);
					c_function.append(StringFormatter.format(
							"        case {}:{\r\n" + 
							"            if(tag == {}){\r\n" + 
							"                {}_set_has_{}(thiz);\r\n" + 
							"                DO_(ReadBytes(s, &thiz->{}));\r\n" + 
							"            }else{\r\n" + 
							"                goto handle_unusual;\r\n" + 
							"            }\r\n" + 
							"            break;\r\n" + 
							"        }\r\n"
							, fieldNumber, tag, proto.getName(), field.getName(), field.getName()));		
				} 
				else if (type == Type.TYPE_STRING) {
					tag = MakeTag(fieldNumber, WireFormat.WIRETYPE_LENGTH_DELIMITED);
					c_function.append(StringFormatter.format(
							"        case {}:{\r\n" + 
							"            if(tag == {}){\r\n" + 
							"                {}_set_has_{}(thiz);\r\n" + 
							"                DO_(ReadString(s, &thiz->{}));\r\n" + 
							"            }else{\r\n" + 
							"                goto handle_unusual;\r\n" + 
							"            }\r\n" + 
							"            break;\r\n" + 
							"        }\r\n"
							, fieldNumber, tag, proto.getName(), field.getName(), field.getName()));			
				} else if (type == Type.TYPE_ENUM) {
					tag = MakeTag(fieldNumber, WireFormat.WIRETYPE_VARINT);
					c_function.append(StringFormatter.format(
							"        case {}:{\r\n" + 
							"            if(tag == {}){\r\n" + 
							"                {}_set_has_{}(thiz);\r\n" + 
							"                DO_(ReadEnum(s, (int32_t*)&thiz->{}));\r\n" + 
							"            }else{\r\n" + 
							"                goto handle_unusual;\r\n" + 
							"            }\r\n" + 
							"            break;\r\n" + 
							"        }\r\n"
							, fieldNumber, tag, proto.getName(), field.getName(), field.getName()));	
				} else if (type == Type.TYPE_UINT32) {
					tag = MakeTag(fieldNumber, WireFormat.WIRETYPE_VARINT);
					c_function.append(StringFormatter.format(
							"        case {}:{\r\n" + 
							"            if(tag == {}){\r\n" + 
							"                {}_set_has_{}(thiz);\r\n" + 
							"                DO_(ReadUint32(s, &thiz->{}));\r\n" + 
							"            }else{\r\n" + 
							"                goto handle_unusual;\r\n" + 
							"            }\r\n" + 
							"            break;\r\n" + 
							"        }\r\n"
							, fieldNumber, tag, proto.getName(), field.getName(), field.getName()));		
				} else if (type == Type.TYPE_UINT64) {
					tag = MakeTag(fieldNumber, WireFormat.WIRETYPE_VARINT);
					c_function.append(StringFormatter.format(
							"        case {}:{\r\n" + 
							"            if(tag == {}){\r\n" + 
							"                {}_set_has_{}(thiz);\r\n" + 
							"                DO_(ReadUint64(s, &thiz->{}));\r\n" + 
							"            }else{\r\n" + 
							"                goto handle_unusual;\r\n" + 
							"            }\r\n" + 
							"            break;\r\n" + 
							"        }\r\n"
							, fieldNumber, tag, proto.getName(), field.getName(), field.getName()));	
				} else if (type == Type.TYPE_INT64) {
					tag = MakeTag(fieldNumber, WireFormat.WIRETYPE_VARINT);
					c_function.append(StringFormatter.format(
							"        case {}:{\r\n" + 
							"            if(tag == {}){\r\n" + 
							"                {}_set_has_{}(thiz);\r\n" + 
							"                DO_(ReadInt64(s, &thiz->{}));\r\n" + 
							"            }else{\r\n" + 
							"                goto handle_unusual;\r\n" + 
							"            }\r\n" + 
							"            break;\r\n" + 
							"        }\r\n"
							, fieldNumber, tag, proto.getName(), field.getName(), field.getName()));	
				} else if (type == Type.TYPE_INT32) {
					tag = MakeTag(fieldNumber, WireFormat.WIRETYPE_VARINT);
					c_function.append(StringFormatter.format(
							"        case {}:{\r\n" + 
							"            if(tag == {}){\r\n" + 
							"                {}_set_has_{}(thiz);\r\n" + 
							"                DO_(ReadInt32(s, &thiz->{}));\r\n" + 
							"            }else{\r\n" + 
							"                goto handle_unusual;\r\n" + 
							"            }\r\n" + 
							"            break;\r\n" + 
							"        }\r\n"
							, fieldNumber, tag, proto.getName(), field.getName(), field.getName()));	
				} else if (type == Type.TYPE_MESSAGE) {
					tag = MakeTag(fieldNumber, WireFormat.WIRETYPE_LENGTH_DELIMITED);
					c_function.append(StringFormatter.format(
							"        case {}:{\r\n" + 
							"            if(tag == {}){\r\n" + 
							"                {}_set_has_{}(thiz);\r\n" + 
							"                if(!thiz->{}._inited){\r\n" + 
							"                    {}_init(&thiz->{});\r\n" + 
							"                }\r\n" +
							"                DO_(ReadMessage(s, (pb_message*)&thiz->{}));\r\n" + 
							"            }else{\r\n" + 
							"                goto handle_unusual;\r\n" + 
							"            }\r\n" + 
							"            break;\r\n" + 
							"        }\r\n"
							, fieldNumber, tag, proto.getName(), field.getName(), 
							field.getName(), typename(field.getTypeName()), field.getName(),
							field.getName()));	
				} else {
					throw new RuntimeException("unsupport " + type);
				}
			}
			c_function.append(
					"        default:{\r\n" + 
					"            handle_unusual:\r\n" + 
					"            if(tag == 0){\r\n" + 
					"                goto success;\r\n" + 
					"            }else{\r\n" + 
					"                DO_(SkipField(s, tag));\r\n" + 
					"            }\r\n" + 
					"            break;\r\n" + 
					"        }\r\n" + 
					"        }\r\n" + 
					"    }\r\n" + 
					"success:\r\n" + 
					"    return TRUE;\r\n" + 
					"failure:\r\n" + 
					"    return FALSE;\r\n" +
                    "#undef DO_\r\n" +
					"}\r\n" + 
					"");
			
			
			c_function.append(StringFormatter.format(
					"void {}_init({}* proto){\r\n" +
					"    proto->_cached_size_ = (size_t)(-1);\r\n" +
					"    proto->_has_bits_[0] = 0;\r\n" +
					"    proto->_inited= TRUE;\r\n" +
					"    proto->_imessage.ByteSizeLong = {}_ByteSizeLong;\r\n" + 
					"    proto->_imessage.GetCachedSize = {}_GetCachedSize;\r\n" + 
					"    proto->_imessage.mergeFromInputStream = {}_mergeFromInputStream;\r\n" + 
					"    proto->_imessage.Serialize = {}_Serialize; \r\n" +
					""
					,
					proto.getName(),proto.getName(), proto.getName(),proto.getName(),proto.getName(), proto.getName()));
	
		
			for (FieldDescriptorProto field : fieldList) {
				Type type = field.getType();
				String defaultValue = field.getDefaultValue();
				boolean hasValue = defaultValue != null && !defaultValue.isEmpty();
				if (type == Type.TYPE_BOOL) {	
					defaultValue = hasValue?defaultValue:"0";
					c_function.append(StringFormatter.format(
							"    proto->{} = {};\r\n",
							field.getName(), Long.valueOf(defaultValue)));	
				} 
				else if (type == Type.TYPE_BYTES) {
					c_function.append(StringFormatter.format(
							"    proto->{}.data = NULL;\r\n" + 
					        "    proto->{}.size = 0;\r\n",
							field.getName(), field.getName()));		
				} 
				else if (type == Type.TYPE_STRING) {
					if(hasValue) {
						c_function.append(StringFormatter.format(
								"    proto->{}.data = (uint8_t*)\"{}\";\r\n" + 
						        "    proto->{}.size = (uint32_t){};\r\n",
								field.getName(), defaultValue, field.getName(), defaultValue.length()));	
					}else {
						c_function.append(StringFormatter.format(
								"    proto->{}.data = NULL;\r\n" + 
						        "    proto->{}.size = 0;\r\n",
								field.getName(), field.getName()));		
					}
				} 
				else if (type == Type.TYPE_ENUM) {
					defaultValue = hasValue?defaultValue:"0";
					c_function.append(StringFormatter.format(
							"    proto->{} = {};\r\n",
							field.getName(), Long.valueOf(defaultValue)));	
				} 
				else if (type == Type.TYPE_UINT32) {
					defaultValue = hasValue?defaultValue:"0";
					c_function.append(StringFormatter.format(
							"    proto->{} = {};\r\n",
							field.getName(), Long.valueOf(defaultValue)));					
				} 
				else if (type == Type.TYPE_UINT64) {
					defaultValue = hasValue?defaultValue:"0";
					c_function.append(StringFormatter.format(
							"    proto->{} = {};\r\n",
							field.getName(), Long.valueOf(defaultValue)));					
				} 
				else if (type == Type.TYPE_INT64) {
					defaultValue = hasValue?defaultValue:"0";
					c_function.append(StringFormatter.format(
							"    proto->{} = {};\r\n",
							field.getName(), Long.valueOf(defaultValue)));								
				} 
				else if (type == Type.TYPE_INT32) {
					defaultValue = hasValue?defaultValue:"0";
					c_function.append(StringFormatter.format(
							"    proto->{} = {};\r\n",
							field.getName(), Long.valueOf(defaultValue)));			
				} 
				else if (type == Type.TYPE_MESSAGE) {
					c_function.append(StringFormatter.format(
							"    proto->{}._inited = FALSE;\r\n",
							field.getName()));
				} 
				else {
					throw new RuntimeException("unsupport " + type);
				}
			}
			c_function.append(
					"}\r\n");	
		}
		logger.info("h_define: \r\n{}", h_define.toString());
		logger.info("h_function: \r\n{}", h_function.toString());
		logger.info("c_function: \r\n{}", c_function.toString());

		H.write(StringFormatter.format(
				"#ifndef pb_{}_H\r\n" + 
				"#define pb_{}_H\r\n" + 
				"\r\n" + 
				"#ifdef __cplusplus\r\n" + 
				"extern \"C\" {\r\n" + 
				"#endif\r\n" + 
				"\r\n" + 
				"#include \"protobuf_c.h\"\r\n" +
				"\r\n" + 
				"#ifndef PBOOL\r\n" + 
				"#define PBOOL int\r\n" + 
				"#endif\r\n",
				fileNameNoExt.toUpperCase(), fileNameNoExt.toUpperCase()));
		H.write(h_define.toString());
		H.write(h_function.toString());
		H.write(StringFormatter.format(
				"\r\n\r\n" + 
				"#ifdef __cplusplus\r\n" + 
				"}\r\n" + 
				"#endif\r\n" + 
				"\r\n" + 
				"#endif // pb_{}_H\r\n\r\n\r\n",
				fileNameNoExt.toUpperCase()));
		
		C.write(StringFormatter.format("#include \"{}\"\r\n", new File(tempH).getName()));
		C.write(c_function.toString());
		C.write("\r\n\r\n");
		H.flush();
		H.close();
		C.flush();
		C.close();
		FileUtils.copyFile(new File(tempC), new File(destC));
		FileUtils.copyFile(new File(tempH), new File(destH));
		logger.info("================== SUCCESS =====================");

	}
	
	 static int MakeTag(int field_number, int type) {
	    return ((field_number) << 3) | (type);
	}

	static String typename(String name) {
		return FilenameUtils.getExtension(name);
	}
}
