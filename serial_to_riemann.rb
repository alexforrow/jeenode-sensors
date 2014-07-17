#!/usr/bin/ruby

require 'rubygems'
require 'riemann/client'

riemann ||= Riemann::Client.new(
        :host => '127.0.0.1',
        :port => 5555
      )

def temperature_state(temp)
	if temp >= 28
		:hot
	elsif temp >= 25
		:warm
	elsif temp >= 21
		:ok
	else
		:cold
	end
end

areas = Hash.new

line_num=0
File.open('/dev/ttyUSB0').each do |line|
	event = nil

	print "#{line_num += 1} #{line}"

	# break up line and check it begins with expected string
	parts = line.split(' ')
	next unless parts.first and parts.shift == 'OK'

	node = parts.shift
	sequence = parts.shift
	cmd = parts.shift

	case cmd
	when 'i'
		# info
		node_id = parts.shift
		onewire_sensors = parts.shift.to_i
		dht_enabled = parts.shift.to_i

		event = {
			:host => "#{node}",
			:service => 'info',
			:metric => onewire_sensors + dht_enabled,
			:description => "Node #{node} (#{node_id}) info, with #{onewire_sensors} onewire sensors, and #{dht_enabled} DHT sensors"
		}
	when 't'
		# temperature
		sensor_id = parts.shift
		temperature = parts.shift.to_f

		event = {
			:host => "#{node}:#{sensor_id}",
			:service => 'temperature',
			:metric => temperature,
			:ttl => 300,
			:state => temperature_state(temperature).to_s
		}
	when 'h'
		# humidity
		sensor_id = parts.shift
		humidity = parts.shift.to_f

		event = {
			:host => "#{node}:#{sensor_id}",
			:service => 'humidity',
			:metric => humidity,
			:ttl => 300,
			:state => 'unknown'
		}
	else
		puts "Unknown line: #{line}"
		event = nil
	end
	
	if event
		puts event.inspect
		riemann << event
	end

	#areas[id] = 0 unless areas[id]
	#areas[id] = areas[id] + 1
	#puts areas.inspect
end
