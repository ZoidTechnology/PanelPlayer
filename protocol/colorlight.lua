devices = {
	['11:22:33:44:55:66'] = 'Receiver',
	['22:22:33:44:55:66'] = 'Sender'
}

packets = {
	[0x01] = 'DISPLAY_FRAME',
	[0x0A] = 'SET_BRIGHTNESS',
	[0x55] = 'IMAGE_DATA'
}

protocol = Proto('Colorlight', 'Colorlight Protocol')

function protocol.dissector(buffer, pinfo, tree)
	local subtree = tree:add(protocol, buffer(), 'Colorlight Data')
	local offset = 0
	local info = ''

	local function add(length, label, format)
		local range = buffer(offset, length)

		if format then
			if format == 'hex' then
				label = label .. ': ' .. range:bytes():tohex()
			else
				label = label .. ': ' .. range[format](range)
			end
		end

		subtree:add(range, label)
		offset = offset + length
	end

	local function add_info(prefix, length, format)
		if prefix then
			info = info .. prefix
		end

		if length then
			local range = buffer(offset, length)

			if format then
				info = info .. range[format](range)
				return
			end

			info = info .. range:bytes():tohex()
		end
	end

	local function get_device(append_address)
		local address = tostring(buffer(offset, 6):ether())
		local name = devices[address]

		if (name == nil) then
			name = 'Unknown'
		end

		if not append_address then
			return name
		end

		return name .. ' (' .. address .. ')'
	end

	pinfo.cols.dst = get_device()
	add(6, 'Destination: ' .. get_device(true))

	pinfo.cols.src = get_device()
	add(6, 'Source: ' .. get_device(true))

	local type = packets[buffer(offset, 1):uint()]

	if (type == nil) then
		type = 'UNKNOWN'
	end

	pinfo.cols.protocol = protocol.name .. '_' .. type
	add_info('Type: ' .. type .. ' (', 1)
	add_info(')')
	add(1, 'Packet type: ' .. type)

	if type == 'DISPLAY_FRAME' then
		add(1, 'Always 0x07', 'hex')
		add(21, 'Always 0x00', 'hex')
		add(1, 'Brightness (linear)', 'uint')
		add(1, 'Always 0x05', 'hex')
		add(1, 'Always 0x00', 'hex')
		add(1, 'Red brightness (linear)', 'uint')
		add(1, 'Green brightness (linear)', 'uint')
		add(1, 'Blue brightness (linear)', 'uint')
		add(buffer:len() - offset, 'Always 0x00')
	end

	if type == 'SET_BRIGHTNESS' then
		add(1, 'Red brightness (non-linear)', 'uint')
		add(1, 'Green brightness (non-linear)', 'uint')
		add(1, 'Blue brightness (non-linear)', 'uint')
		add(1, 'Always 0xFF', 'hex')
		add(buffer:len() - offset, 'Always 0x00')
	end

	if type == 'IMAGE_DATA' then
		add_info(', Row number: ', 2, 'uint')
		add(2, 'Row number', 'uint')
		add(2, 'Pixel horizontal offset', 'uint')
		add(2, 'Pixel count', 'uint')
		add(1, 'Always 0x08', 'hex')
		add(1, 'Always 0x88', 'hex')
		add(buffer:len() - offset, 'Pixel data')
	end

	pinfo.cols.info = info
end

table = DissectorTable.get('wtap_encap')
table:add(1, protocol)